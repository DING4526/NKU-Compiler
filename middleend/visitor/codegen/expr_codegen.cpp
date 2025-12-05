#include <middleend/visitor/codegen/ast_codegen.h>

namespace ME
{
    void ASTCodeGen::visit(FE::AST::LeftValExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 生成左值表达式的取址/取值 IR
        // 查找变量位置（全局或局部），处理数组下标/GEP，必要时发出load

        // TODO("Lab3-2: Implement LeftValExpr IR generation");
        (void)m;
        // 基础要求：仅支持标量变量；不支持数组索引
        ASSERT((!node.indices || node.indices->empty()) && "Array indexing not supported in base requirement");

        Operand* ptr = nullptr;

        // 先查局部（alloca 的 ptr reg）
        size_t reg = name2reg.getReg(node.entry);
        if (reg != static_cast<size_t>(-1))
        {
            ptr = getRegOperand(reg);
        }
        else
        {
            // 否则一律按“全局变量”处理：直接用名字拿 @a 这种 GlobalOperand
            ptr = getGlobalOperand(node.entry->getName());
        }

        lval2ptr[&node] = ptr;

        DataType ty = convert(node.attr.val.value.type);
        if (ty == DataType::I1) ty = DataType::I32; // bool 当 i32 处理（基础要求）
        ASSERT(ty == DataType::I32 && "Base requirement: no float");

        size_t resReg = getNewRegId();
        insert(createLoadInst(DataType::I32, ptr, resReg));
    }

    void ASTCodeGen::visit(FE::AST::LiteralExpr& node, Module* m)
    {
        (void)m;

        size_t reg = getNewRegId();
        switch (node.literal.type->getBaseType())
        {
            case FE::AST::Type_t::INT:
            case FE::AST::Type_t::LL:  // treat as I32
            {
                int             val  = node.literal.getInt();
                ArithmeticInst* inst = createArithmeticI32Inst_ImmeAll(Operator::ADD, val, 0, reg);  // reg = val + 0
                insert(inst);
                break;
            }
            case FE::AST::Type_t::FLOAT:
            {
                float           val  = node.literal.getFloat();
                ArithmeticInst* inst = createArithmeticF32Inst_ImmeAll(Operator::FADD, val, 0, reg);  // reg = val + 0
                insert(inst);
                break;
            }
            default: ERROR("Unsupported literal type");
        }
    }

    void ASTCodeGen::visit(FE::AST::UnaryExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 生成一元运算的 IR（访问操作数、必要的类型转换、发出运算指令）

        // TODO("Lab3-2: Implement UnaryExpr IR generation");

        // 基础要求：只处理 + - !，且只在 int/bool 上
        handleUnaryCalc(*node.expr, node.op, curBlock, m);

        // 结果类型标记：对 ! 结果是 i1；对 +/- 结果是 i32
        // （后续 Binary/If 要靠 attr.type 做转换）
        // 这里不强行改 attr，假定 checker 已算好类型；如果 checker 没算，可按 op 手动设置（略）
    }

    void ASTCodeGen::handleAssign(FE::AST::LeftValExpr& lhs, FE::AST::ExprNode& rhs, Module* m)
    {
        // TODO(Lab 3-2): 生成赋值语句的 IR（计算右值、类型转换、store 到左值地址）

        // TODO("Lab3-2: Implement assignment IR generation");
        // 1) 计算 lhs 地址：强制 lhs.isLval = true 走“取址”逻辑
        bool old = lhs.isLval;
        lhs.isLval = true;
        apply(*this, lhs, m);
        lhs.isLval = old;

        auto it = lval2ptr.find(&lhs);
        ASSERT(it != lval2ptr.end());
        Operand* ptr = it->second;

        // 2) 计算 rhs 值
        apply(*this, rhs, m);
        size_t rhsReg = getMaxReg();
        DataType rhsTy = convert(rhs.attr.val.value.type);

        // 3) 转成 i32（基础要求）
        if (rhsTy == DataType::I1)
        {
            auto conv = createTypeConvertInst(DataType::I1, DataType::I32, rhsReg);
            for (auto* inst : conv) insert(inst);
            rhsReg = getMaxReg();
            rhsTy = DataType::I32;
        }
        ASSERT(rhsTy == DataType::I32 && "Base requirement: no float assignment");

        // 4) store
        insert(createStoreInst(DataType::I32, rhsReg, ptr));

        // 5) 赋值表达式的值等于 rhs（C 语义）
        // 这里为了让上层能拿到“表达式值”，补一个 load/store-less 的“复制”：
        // 最简单：再 load 回来作为该表达式结果
        size_t resReg = getNewRegId();
        insert(createLoadInst(DataType::I32, ptr, resReg));
    }

    void ASTCodeGen::handleLogicalAnd(
        FE::AST::BinaryExpr& node, FE::AST::ExprNode& lhs, FE::AST::ExprNode& rhs, Module* m)
    {
        // TODO(Lab 3-2): 生成短路与的基本块与条件分支

        // TODO("Lab3-2: Implement logical AND codegen");
        // 目标：lhs 为真才计算 rhs
        // node.trueTar/node.falseTar 已由外层设置
        ASSERT(node.trueTar != static_cast<size_t>(-1) && node.falseTar != static_cast<size_t>(-1));

        // 建一个 rhs block 作为 lhs=true 的落点
        Block* rhsBlock = createBlock();
        size_t rhsLabel = rhsBlock->blockId;

        // lhs: true -> rhsLabel, false -> node.falseTar
        lhs.trueTar = rhsLabel;
        lhs.falseTar = node.falseTar;
        apply(*this, lhs, m);

        // 进入 rhs block 生成 rhs，去往 node.trueTar / node.falseTar
        enterBlock(rhsBlock);
        rhs.trueTar = node.trueTar;
        rhs.falseTar = node.falseTar;
        apply(*this, rhs, m);
    }

    void ASTCodeGen::handleLogicalOr(
        FE::AST::BinaryExpr& node, FE::AST::ExprNode& lhs, FE::AST::ExprNode& rhs, Module* m)
    {
        // TODO(Lab 3-2): 生成短路或的基本块与条件分支

        // TODO("Lab3-2: Implement logical OR codegen");
        // 目标：lhs 为假才计算 rhs
        ASSERT(node.trueTar != static_cast<size_t>(-1) && node.falseTar != static_cast<size_t>(-1));

        Block* rhsBlock = createBlock();
        size_t rhsLabel = rhsBlock->blockId;

        // lhs: true -> node.trueTar, false -> rhsLabel
        lhs.trueTar = node.trueTar;
        lhs.falseTar = rhsLabel;
        apply(*this, lhs, m);

        enterBlock(rhsBlock);
        rhs.trueTar = node.trueTar;
        rhs.falseTar = node.falseTar;
        apply(*this, rhs, m);
    }

    void ASTCodeGen::visit(FE::AST::BinaryExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 生成二元表达式 IR（含赋值、逻辑与/或、算术/比较）

        // TODO("Lab3-2: Implement BinaryExpr IR generation");
        auto op = node.op;

        // 1) 赋值
        if (op == FE::AST::Operator::ASSIGN)
        {
            auto* l = dynamic_cast<FE::AST::LeftValExpr*>(node.lhs);
            ASSERT(l && "LHS of assignment must be LeftValExpr");
            handleAssign(*l, *node.rhs, m);
            return;
        }

        // 2) 逻辑与/或（短路）：要求外层设置 node.trueTar/falseTar
        if (op == FE::AST::Operator::AND)
        {
            handleLogicalAnd(node, *node.lhs, *node.rhs, m);
            return;
        }
        if (op == FE::AST::Operator::OR)
        {
            handleLogicalOr(node, *node.lhs, *node.rhs, m);
            return;
        }

        // 3) 其他二元：算术/比较
        handleBinaryCalc(*node.lhs, *node.rhs, op, curBlock, m);

        // 如果这是一个“条件上下文”里的表达式（trueTar/falseTar 已设置），则在这里发出 brcond
        // - handleBinaryCalc 对比较类会产生 i1；对算术类产生 i32
        if (node.trueTar != static_cast<size_t>(-1) && node.falseTar != static_cast<size_t>(-1))
        {
            size_t condReg = getMaxReg();
            DataType t = convert(node.attr.val.value.type);

            // 条件需要 i1
            if (t == DataType::I32)
            {
                auto conv = createTypeConvertInst(DataType::I32, DataType::I1, condReg);
                for (auto* inst : conv) insert(inst);
                condReg = getMaxReg();
            }
            else if (t == DataType::I1)
            {
                // ok
            }
            else
            {
                ERROR("Base requirement: no float in condition");
            }

            insert(createBranchInst(condReg, node.trueTar, node.falseTar));
        }
    }

    void ASTCodeGen::visit(FE::AST::CallExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 生成函数调用 IR（准备参数、可选返回寄存器、发出call）

        // TODO("Lab3-2: Implement CallExpr IR generation");
        std::string fname = node.func->getName();

        // 从 funcDecls 找到声明（语义阶段应该已经填好）
        auto it = funcDecls.find(node.func);
        if (it == funcDecls.end())
        {
            // 允许库函数：libFuncRegister 已经加到 module，但 funcDecls map 未必含它
            // 这里简单放行：按名字处理返回类型（基础要求只需要 getint/putint 这类）
        }

        // 生成实参
        CallInst::argList args;
        if (node.args)
        {
            for (auto* a : *node.args)
            {
                apply(*this, *a, m);
                size_t areg = getMaxReg();
                DataType at = convert(a->attr.val.value.type);

                // 参数按 i32 处理（bool -> i32）
                if (at == DataType::I1)
                {
                    auto conv = createTypeConvertInst(DataType::I1, DataType::I32, areg);
                    for (auto* inst : conv) insert(inst);
                    areg = getMaxReg();
                    at = DataType::I32;
                }
                ASSERT(at == DataType::I32 && "Base requirement: only int/bool args");

                args.push_back({DataType::I32, getRegOperand(areg)});
            }
        }

        // 返回类型：尽量从 decl 取；否则按库函数名猜一个最小集合
        DataType rt = DataType::I32;
        if (it != funcDecls.end())
        {
            rt = convert(it->second->retType);
            if (rt == DataType::I1) rt = DataType::I32;
        }
        else
        {
            if (fname == "putint" || fname == "putch" || fname == "putarray" ||
                fname == "_sysy_starttime" || fname == "_sysy_stoptime" ||
                fname == "llvm.memset.p0.i32")
                rt = DataType::VOID;
            else
                rt = DataType::I32; // getint/getch 等
        }

        if (rt == DataType::VOID)
        {
            insert(createCallInst(DataType::VOID, fname, args));
        }
        else
        {
            ASSERT(rt == DataType::I32 && "Base requirement: only int return");
            size_t resReg = getNewRegId();
            insert(createCallInst(DataType::I32, fname, args, resReg));
        }
    }

    void ASTCodeGen::visit(FE::AST::CommaExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 依序生成逗号表达式每个子表达式的 IR

        // TODO("Lab3-2: Implement CommaExpr IR generation");
        ASSERT(node.exprs && "CommaExpr exprs is null");
        for (auto* e : *node.exprs)
        {
            if (!e) continue;
            apply(*this, *e, m);
        }
    }
}  // namespace ME
