#include <middleend/visitor/codegen/ast_codegen.h>
#include <debug.h>

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
        // 基础要求：仅支持 int/bool 标量局部变量；不支持数组、不支持浮点、不支持初始化列表
        DataType base = convert(node.type);
        if (base == DataType::I1) base = DataType::I32;
        ASSERT(base == DataType::I32 && "Base requirement: only support int/bool local scalar");

        for (auto* d : *node.decls)
        {
            auto* lv = dynamic_cast<FE::AST::LeftValExpr*>(d->lval);
            ASSERT(lv && "VarDeclarator lval must be LeftValExpr");
            ASSERT((!lv->indices || lv->indices->empty()) && "Array local decl not supported in base requirement");

            // 1) alloca
            size_t ptrReg = getNewRegId();
            
            Instruction* allocaInst = createAllocaInst(DataType::I32, ptrReg);

            Block* entryBlock = getBlock(0);        // 约定：blockId=0 是入口块
            auto& insts = entryBlock->insts;        // std::list<Instruction*>

            // 简单做法：所有局部变量的 alloca 都插到块最前面
            insts.push_front(allocaInst);

            // 2) 记录符号 -> ptr reg
            name2reg.addSymbol(lv->entry, ptrReg);

            // 3) 初始化：无初始化则 store 0；有初始化则计算表达式并转换到 i32 再 store
            if (!d->init)
            {
                insert(createStoreInst(DataType::I32, getImmeI32Operand(0), getRegOperand(ptrReg)));
            }
            else
            {
                ASSERT(d->init->singleInit && "InitializerList not supported (base requirement)");
                auto* init = dynamic_cast<FE::AST::Initializer*>(d->init);
                ASSERT(init && "Local init should be Initializer");

                apply(*this, *init->init_val, m);
                size_t rhsReg = getMaxReg();
                DataType rhsTy = convert(init->init_val->attr.val.value.type);
                if (rhsTy == DataType::I1)
                {
                    // bool -> i32
                    auto conv = createTypeConvertInst(DataType::I1, DataType::I32, rhsReg);
                    for (auto* inst : conv) insert(inst);
                    rhsReg = getMaxReg();
                    rhsTy = DataType::I32;
                }
                ASSERT(rhsTy == DataType::I32 && "Base requirement: no float");

                insert(createStoreInst(DataType::I32, rhsReg, getRegOperand(ptrReg)));
            }
        }
    }
}  // namespace ME
