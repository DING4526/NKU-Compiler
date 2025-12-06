#include <middleend/visitor/codegen/ast_codegen.h>
#include <debug.h>

namespace
{
    static int64_t prod(const std::vector<int>& dims)
    {
        int64_t p = 1;
        for (int d : dims) p *= d;
        return p;
    }
}

namespace ME
{
    void ASTCodeGen::visit(FE::AST::Initializer& node, Module* m)
    {
        (void)m;
        ERROR("Initializer should not appear here, at line %d", node.line_num);
    }
    void ASTCodeGen::visit(FE::AST::InitializerList& node, Module* m)
    {
        (void)m;
        ERROR("InitializerList should not appear here, at line %d", node.line_num);
    }
    void ASTCodeGen::visit(FE::AST::VarDeclarator& node, Module* m)
    {
        (void)m;
        ERROR("VarDeclarator should not appear here, at line %d", node.line_num);
    }
    void ASTCodeGen::visit(FE::AST::ParamDeclarator& node, Module* m)
    {
        (void)m;
        ERROR("ParamDeclarator should not appear here, at line %d", node.line_num);
    }

    void ASTCodeGen::visit(FE::AST::VarDeclaration& node, Module* m)
    {
        // TODO(Lab 3-2): 生成变量声明 IR（alloca、数组零初始化、可选初始化表达式）

        // TODO("Lab3-2: Implement VarDeclaration IR generation");
        (void)m;
        DataType base = convert(node.type);
        // 你原来把 bool 当 i32，继续保持
        if (base == DataType::I1) base = DataType::I32;
        ASSERT(base == DataType::I32 || base == DataType::F32);

        Block* entryBlock = getBlock(0);

        for (auto* d : *node.decls)
        {
            auto* lv = dynamic_cast<FE::AST::LeftValExpr*>(d->lval);
            ASSERT(lv && "VarDeclarator lval 必须是 LeftValExpr");

            // 维度：来自语义属性（推荐）或来自 lv->indices（声明时的 [] 也可能走 indices）
            // 你给的 VarAttr 风格是符号表存 dims，因此这里建议从语义符号属性取。
            // 由于你没给“局部符号表 -> VarAttr”的直接入口，这里用简单策略：
            //   - 若 lv->indices 用于声明维度：要求它们是 constexpr，取 attr.val.value.getInt()
            std::vector<int> dims;
            if (lv->indices && !lv->indices->empty())
            {
                for (auto* dimExpr : *lv->indices)
                {
                    ASSERT(dimExpr->attr.val.isConstexpr && "数组声明维度必须是编译期常量");
                    int dim = dimExpr->attr.val.getInt();
                    ASSERT(dim > 0);
                    dims.push_back(dim);
                }
            }

            // 1) alloca（标量/数组）
            size_t ptrReg = getNewRegId();
            if (dims.empty())
            {
                entryBlock->insts.push_front(createAllocaInst(base, ptrReg));
            }
            else
            {
                entryBlock->insts.push_front(createAllocaInst(base, ptrReg, dims));
                // 保存维度信息，供 LeftValExpr 做 GEP
                reg2attr[ptrReg] = FE::AST::VarAttr(node.type, node.isConstDecl, /*level*/-1, dims, {});
            }

            name2reg.addSymbol(lv->entry, ptrReg);

            // 2) 初始化
            if (dims.empty())
            {
                // 标量：无 init -> 0，有 init -> 计算表达式并 cast
                if (!d->init)
                {
                    if (base == DataType::I32) insert(createStoreInst(DataType::I32, getImmeI32Operand(0), getRegOperand(ptrReg)));
                    else                       insert(createStoreInst(DataType::F32, getImmeF32Operand(0.0f), getRegOperand(ptrReg)));
                }
                else
                {
                    ASSERT(d->init->singleInit && "标量初始化不应是 InitializerList");
                    auto* init = dynamic_cast<FE::AST::Initializer*>(d->init);
                    ASSERT(init && init->init_val);

                    apply(*this, *init->init_val, m);
                    size_t rhsReg = queryExprReg(init->init_val);
                    DataType rhsTy = queryExprType(init->init_val);

                    if (rhsTy == DataType::I1) { rhsReg = castTo(DataType::I1, DataType::I32, rhsReg); rhsTy = DataType::I32; }
                    if (rhsTy != base) rhsReg = castTo(rhsTy, base, rhsReg);

                    insert(createStoreInst(base, rhsReg, getRegOperand(ptrReg)));
                }
            }
            else
            {
                // 数组：先 memset 0，再按 initList 或 initializerlist store
                int64_t nElem = prod(dims);
                int bytes = static_cast<int>(nElem * ((base == DataType::F32) ? 4 : 4));

                // memset(ptr, 0, bytes, false)
                insert(createCallInst(DataType::VOID, "llvm.memset.p0.i32",
                    {{DataType::PTR, getRegOperand(ptrReg)},
                    {DataType::I8,  getImmeI32Operand(0)},
                    {DataType::I32, getImmeI32Operand(bytes)},
                    {DataType::I1,  getImmeI32Operand(0 ? 1 : 0)}}));

                if (!d->init) continue;

                // 优先：如果语义阶段已把 initList 展平到符号属性（理想）
                // 由于这里暂时拿不到 entry->VarAttr，我们退化：支持 InitializerList 递归生成 store
                if (!d->init->singleInit)
                {
                    auto* il = dynamic_cast<FE::AST::InitializerList*>(d->init);
                    ASSERT(il && "数组初始化应为 InitializerList");
                    // 这里给出最小实现：只支持一维平铺 {a,b,c,...}
                    // 多维递归展开你可以后续补（如果你确认 init_list 的嵌套结构都保留）
                    int idx = 0;
                    for (auto* item : *il->init_list)
                    {
                        if (!item) continue;
                        if (!item->singleInit) ERROR("多维嵌套初始化列表：请按需要递归展开（本版本先不展开）");

                        auto* ini = dynamic_cast<FE::AST::Initializer*>(item);
                        ASSERT(ini && ini->init_val);
                        apply(*this, *ini->init_val, m);
                        size_t vreg = queryExprReg(ini->init_val);
                        DataType vt = queryExprType(ini->init_val);
                        if (vt == DataType::I1) { vreg = castTo(DataType::I1, DataType::I32, vreg); vt = DataType::I32; }
                        if (vt != base) vreg = castTo(vt, base, vreg);

                        // 生成 GEP(ptr, [idx])（仅一维）
                        size_t gepReg = getNewRegId();
                        insert(createGEP_I32Inst(base, getRegOperand(ptrReg), dims, {getImmeI32Operand(idx)}, gepReg));
                        insert(createStoreInst(base, vreg, getRegOperand(gepReg)));

                        idx++;
                        if (idx >= nElem) break;
                    }
                }
                else
                {
                    // int a[3] = expr; 这种不合法或当作 a[0]=expr（按你需求定）
                    ERROR("数组不支持 singleInit 形式初始化");
                }
            }
        }
    }
}  // namespace ME
