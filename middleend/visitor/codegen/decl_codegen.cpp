#include <middleend/visitor/codegen/ast_codegen.h>
#include <debug.h>

namespace ME
{
    namespace
    {
        static int64_t prod(const std::vector<int>& dims)
        {
            int64_t p = 1;
            for (int d : dims) p *= d;
            return p;
        }
        static int64_t prodFrom(const std::vector<int>& dims, int start)
{
    int64_t p = 1;
    for (int i = start; i < (int)dims.size(); ++i) p *= dims[i];
    return p;
}
    }

// 支持 brace elision：中间维度允许出现 singleInit（如 {{0,9}, 8, 3}）
// baseIndex: 本聚合在扁平数组中的起点
// total: 本聚合最多能写多少个“标量元素”
// cursor: 相对本聚合的写入游标 [0,total)
void ASTCodeGen::emitArrayInitFill(FE::AST::InitializerList* il,
                                  int dim,
                                  const std::vector<int>& dims,
                                  int64_t baseIndex,
                                  int64_t total,
                                  int64_t& cursor,
                                  DataType base,
                                  size_t ptrReg,
                                  Module* m)
{
    ASSERT(il);
    int rank = (int)dims.size();
    ASSERT(rank >= 1 && dim >= 0 && dim < rank);

    // 当前维度一个“子聚合”占多少标量
    int64_t subSize = (dim < rank - 1) ? prodFrom(dims, dim + 1) : 1;

    for (auto* child : *il->init_list)
    {
        if (!child) continue;
        if (cursor >= total) break; // 多余初始化丢弃，符合 C 规则

        if (!child->singleInit)
        {
            // 子列表：对齐到 subSize 边界（核心：brace elision 的规则）
            if (dim < rank - 1 && (cursor % subSize) != 0)
                cursor += (subSize - cursor % subSize);

            if (cursor >= total) break;

            auto* subList = dynamic_cast<FE::AST::InitializerList*>(child);
            ASSERT(subList);

            if (dim < rank - 1)
            {
                // 递归填充一个子聚合
                int64_t subCursor = 0;
                emitArrayInitFill(subList,
                                  dim + 1,
                                  dims,
                                  baseIndex + cursor,
                                  subSize,
                                  subCursor,
                                  base,
                                  ptrReg,
                                  m);

                // 子聚合结束后，父游标直接跳过整个子聚合
                cursor += subSize;
            }
            else
            {
                // 最后一维遇到额外 {}（如 {{}}），当作最后一维继续填即可
                emitArrayInitFill(subList,
                                  dim,
                                  dims,
                                  baseIndex,
                                  total,
                                  cursor,
                                  base,
                                  ptrReg,
                                  m);
            }
        }
        else
        {
            // 标量：按行主序连续填
            auto* ini = dynamic_cast<FE::AST::Initializer*>(child);
            ASSERT(ini && ini->init_val);

            emitArrayScalarInitAtIndex(ini->init_val,
                                       baseIndex + cursor,
                                       base,
                                       ptrReg,
                                       dims,
                                       m);
            cursor += 1;
        }
    }
}
    void ASTCodeGen::emitArrayScalarInitAtIndex(FE::AST::ExprNode*    expr,
                                                int64_t               flatIndex,
                                                DataType              base,
                                                size_t                ptrReg,
                                                const std::vector<int>& dims,
                                                Module*               m)
    {
        ASSERT(expr);
        // 求值
        apply(*this, *expr, m);
        size_t   vreg = queryExprReg(expr);
        DataType vt   = queryExprType(expr);

        // bool -> i32
        if (vt == DataType::I1)
        {
            vreg = castTo(DataType::I1, DataType::I32, vreg);
            vt   = DataType::I32;
        }
        // 需要的话做数值转换
        if (vt != base)
            vreg = castTo(vt, base, vreg);

        // 生成下标寄存器（i32）
        ASSERT(flatIndex >= 0 && flatIndex <= INT32_MAX);
        size_t idxReg = getNewRegId();
        insert(createArithmeticI32Inst_ImmeAll(Operator::ADD,
                                               static_cast<int>(flatIndex),
                                               0,
                                               idxReg));

    // GEP：已经是“扁平下标”了，这里要把它当成一维 i32 数组来算，
    //      所以 dims 传空，让 createGEP_I32Inst 生成：
    //      getelementptr i32, ptr %ptrReg, i32 flatIndex
    (void)dims;
    size_t gepReg = getNewRegId();
    insert(createGEP_I32Inst(
        base,                     // 元素类型：i32 / f32
        getRegOperand(ptrReg),    // alloca 得到的指针
        /*dims*/ {},              // ★★ 关键改动：不再传 dims
        { getRegOperand(idxReg) },
        gepReg));

    insert(createStoreInst(base, vreg, getRegOperand(gepReg)));
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

        // 从第 0 维开始递归展开
        int64_t cursor = 0;
        emitArrayInitFill(il, 0, dims, 0, prod(dims), cursor, base, ptrReg, m);
    }
    else
    {
        // 按 SysY 习惯，这种写法一般不会出现，直接给出错误（或者你愿意，也可以当 a[0] = expr 处理）
        ERROR("数组不支持 singleInit 形式初始化");
    }
            }
        }
    }
}  // namespace ME
