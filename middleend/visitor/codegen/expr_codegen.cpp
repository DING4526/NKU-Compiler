#include <middleend/visitor/codegen/ast_codegen.h>

namespace ME
{
    void ASTCodeGen::visit(FE::AST::LeftValExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 生成左值表达式的取址/取值 IR
        // 查找变量位置（全局或局部），处理数组下标/GEP，必要时发出load

        // TODO("Lab3-2: Implement LeftValExpr IR generation");
        (void)m;
        // 1) 找到 base ptr：局部优先，其次全局
        size_t localPtrReg = name2reg.getReg(node.entry);
        Operand* basePtr = nullptr;
        bool isLocal = (localPtrReg != static_cast<size_t>(-1));
        if (isLocal)
            basePtr = getRegOperand(localPtrReg);      // alloca 返回的指针
        else
            basePtr = getGlobalOperand(node.entry->getName());  // 全局符号

        // 2) 取数组维度信息（供 GEP 用）
        std::vector<int> dims;
        if (isLocal) {
            auto ita = reg2attr.find(localPtrReg);
            if (ita != reg2attr.end())
                dims = ita->second.arrayDims;
        } else {
            auto itg = glbSymbols.find(node.entry);
            if (itg != glbSymbols.end())
                dims = itg->second.arrayDims;
        }

        Operand* elemPtr = basePtr;

        // 3) 如果有下标 => 生成 GEP
        if (node.indices && !node.indices->empty())
        {
            ASSERT(!dims.empty() && "有 indices 但没维度信息，记得在 alloca/形参建 reg2attr");

            std::vector<Operand*> idxOps;
            idxOps.reserve(node.indices->size());

            for (auto* idxExpr : *node.indices)
            {
                apply(*this, *idxExpr, m);
                size_t ir = queryExprReg(idxExpr);
                DataType it = queryExprType(idxExpr);

                // 下标统一转 i32
                if (it != DataType::I32)
                    ir = castTo(it, DataType::I32, ir);

                idxOps.push_back(getRegOperand(ir));
            }

            size_t gepReg = getNewRegId();
            // 元素类型从语义属性拿
            DataType elemTy = convert(node.attr.val.value.type);
            if (elemTy == DataType::I1) elemTy = DataType::I32;

            insert(createGEP_I32Inst(elemTy, basePtr, dims, idxOps, gepReg));
            elemPtr = getRegOperand(gepReg);
        }

        // 4) 不管怎样，记录“这个左值的地址”
        lval2ptr[&node] = elemPtr;

        // 5) 表达式值：统一生成（这样 queryExprReg 永远可用）
        DataType valTy = convert(node.attr.val.value.type);
        if (valTy == DataType::I1) valTy = DataType::I32;

        // 如果语义上是“指针值”（比如数组形参、a[] 退化），直接用地址寄存器作为值
        if (valTy == DataType::PTR && isLocal)
        {
            recordExprResult(&node, localPtrReg, DataType::PTR);
            return;
        }

        // 普通标量：load
        if (valTy == DataType::I32 || valTy == DataType::F32)
        {
            size_t resReg = getNewRegId();
            insert(createLoadInst(valTy, elemPtr, resReg));
            recordExprResult(&node, resReg, valTy);
            return;
        }
    }

    void ASTCodeGen::visit(FE::AST::LiteralExpr& node, Module* m)
    {
        (void)m;

        size_t reg = getNewRegId();
        switch (node.literal.type->getBaseType())
        {
            case FE::AST::Type_t::INT:
            case FE::AST::Type_t::LL:
            {
                int val = node.literal.getInt();
                insert(createArithmeticI32Inst_ImmeAll(Operator::ADD, val, 0, reg));
                recordExprResult(&node, reg, DataType::I32);
                break;
            }
            case FE::AST::Type_t::FLOAT:
            {
                float val = node.literal.getFloat();
                insert(createArithmeticF32Inst_ImmeAll(Operator::FADD, val, 0, reg));
                recordExprResult(&node, reg, DataType::F32);
                break;
            }
            default: ERROR("不支持的字面量类型");
        }
    }

    void ASTCodeGen::visit(FE::AST::UnaryExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 生成一元运算的 IR（访问操作数、必要的类型转换、发出运算指令）

        // TODO("Lab3-2: Implement UnaryExpr IR generation");

        // 仍沿用你现有的 handleUnaryCalc
        handleUnaryCalc(*node.expr, node.op, curBlock, m);

        // handleUnaryCalc 最后一定插入一条产生新 reg 的指令
        size_t res = getMaxReg();

        // 类型：从语义属性拿最稳；若语义没填好，也可根据 op 推断
        DataType t = convert(node.attr.val.value.type);
        if (t == DataType::I1) { recordExprResult(&node, res, DataType::I1); return; }
        if (t == DataType::I32 || t == DataType::F32)
        {
            recordExprResult(&node, res, t);
            return;
        }

        // 兜底：! -> i1，+/- -> 跟 operand 同类
        if (node.op == FE::AST::Operator::NOT)
            recordExprResult(&node, res, DataType::I1);
        else
            recordExprResult(&node, res, DataType::I32);
    }

    void ASTCodeGen::handleAssign(FE::AST::LeftValExpr& lhs, FE::AST::ExprNode& rhs, Module* m)
    {
        // TODO(Lab 3-2): 生成赋值语句的 IR（计算右值、类型转换、store 到左值地址）

        // TODO("Lab3-2: Implement assignment IR generation");
        // 1) lhs 取址
        bool old = lhs.isLval;
        lhs.isLval = true;
        apply(*this, lhs, m);
        lhs.isLval = old;

        auto it = lval2ptr.find(&lhs);
        ASSERT(it != lval2ptr.end());
        Operand* ptr = it->second;

        // 2) rhs 求值：禁止 getMaxReg 猜
        apply(*this, rhs, m);
        size_t rhsReg = queryExprReg(&rhs);
        DataType rhsTy = queryExprType(&rhs);

        // 3) lhs 元素类型（标量/数组元素）
        DataType lhsTy = convert(lhs.attr.val.value.type);
        if (lhsTy == DataType::I1) lhsTy = DataType::I32; // 保持原约定

        // 4) rhs 转 lhs 类型
        if (rhsTy == DataType::I1) rhsTy = DataType::I32; // bool 当 i32
        if (rhsTy != lhsTy)
            rhsReg = castTo(rhsTy, lhsTy, rhsReg);

        // 5) store
        insert(createStoreInst(lhsTy, rhsReg, ptr));

        // 6) 赋值表达式的值：再 load 回来（简单且正确）
        size_t resReg = getNewRegId();
        insert(createLoadInst(lhsTy, ptr, resReg));
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

        // 如果 lhs 没在自身里发 terminator（比如是普通表达式），这里补一个 br
        if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())){
            size_t   condReg = getMaxReg();
            DataType t       = convert(lhs.attr.val.value.type);

            if (t == DataType::I32){
                auto conv = createTypeConvertInst(DataType::I32, DataType::I1, condReg);
                for (auto* inst : conv) insert(inst);
                condReg = getMaxReg();
            }
            else if (t != DataType::I1){
                ERROR("Logical AND lhs must be int/bool");
            }

            insert(createBranchInst(condReg, rhsLabel, node.falseTar));
        }

        // 进入 rhs block 生成 rhs，去往 node.trueTar / node.falseTar
        enterBlock(rhsBlock);
        rhs.trueTar = node.trueTar;
        rhs.falseTar = node.falseTar;
        apply(*this, rhs, m);

        if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())){
            size_t   condReg = getMaxReg();
            DataType t       = convert(rhs.attr.val.value.type);

            if (t == DataType::I32){
                auto conv = createTypeConvertInst(DataType::I32, DataType::I1, condReg);
                for (auto* inst : conv) insert(inst);
                condReg = getMaxReg();
            }
            else if (t != DataType::I1){
                ERROR("Logical AND rhs must be int/bool");
            }

            insert(createBranchInst(condReg, node.trueTar, node.falseTar));
        }
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

        if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())){
            size_t   condReg = getMaxReg();
            DataType t       = convert(lhs.attr.val.value.type);

            if (t == DataType::I32){
                auto conv = createTypeConvertInst(DataType::I32, DataType::I1, condReg);
                for (auto* inst : conv) insert(inst);
                condReg = getMaxReg();
            }
            else if (t != DataType::I1){
                ERROR("Logical OR lhs must be int/bool");
            }

            insert(createBranchInst(condReg, node.trueTar, rhsLabel));
        }

        enterBlock(rhsBlock);
        rhs.trueTar = node.trueTar;
        rhs.falseTar = node.falseTar;
        apply(*this, rhs, m);

        if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())){
            size_t   condReg = getMaxReg();
            DataType t       = convert(rhs.attr.val.value.type);

            if (t == DataType::I32){
                auto conv = createTypeConvertInst(DataType::I32, DataType::I1, condReg);
                for (auto* inst : conv) insert(inst);
                condReg = getMaxReg();
            }
            else if (t != DataType::I1){
                ERROR("Logical OR rhs must be int/bool");
            }

            insert(createBranchInst(condReg, node.trueTar, node.falseTar));
        }
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
            ASSERT(l && "赋值左值必须是 LeftValExpr");

            // 先算出左值元素类型
            DataType lhsTy = convert(l->attr.val.value.type);
            if (lhsTy == DataType::I1) lhsTy = DataType::I32;

            // 生成 store + load
            handleAssign(*l, *node.rhs, m);
            size_t valReg = getMaxReg();      // handleAssign 最后那个 load 的结果寄存器

            recordExprResult(&node, valReg, lhsTy);

            // 如果这个赋值表达式是“条件上下文”，还要发 br
            if (node.trueTar != static_cast<size_t>(-1) &&
                node.falseTar != static_cast<size_t>(-1))
            {
                size_t condReg = valReg;
                DataType ct = lhsTy;

                // 条件统一转成 i1
                if (ct != DataType::I1)
                    condReg = castTo(ct, DataType::I1, condReg);

                insert(createBranchInst(condReg, node.trueTar, node.falseTar));
            }
            return;
        }

        // 2) 短路逻辑
        if (op == FE::AST::Operator::AND) { handleLogicalAnd(node, *node.lhs, *node.rhs, m); return; }
        if (op == FE::AST::Operator::OR)  { handleLogicalOr(node, *node.lhs, *node.rhs, m);  return; }

        // 3) 普通二元（算术/比较）：依旧走 handleBinaryCalc
        handleBinaryCalc(*node.lhs, *node.rhs, op, curBlock, m);

        size_t res = getMaxReg();
        DataType t = convert(node.attr.val.value.type);
        if (t == DataType::I1 || t == DataType::I32 || t == DataType::F32)
            recordExprResult(&node, res, t);

        // 4) 如果当前处在“条件上下文”，补 brcond（注意：condReg 查表，不再猜）
        if (node.trueTar != static_cast<size_t>(-1) && node.falseTar != static_cast<size_t>(-1))
        {
            // 条件表达式的寄存器，优先从表里拿；若没记录（理论不该），再退化到 getMaxReg
            size_t condReg = queryExprReg(&node);
            DataType ct    = queryExprType(&node);

            if (ct != DataType::I1)
                condReg = castTo(ct, DataType::I1, condReg);

            insert(createBranchInst(condReg, node.trueTar, node.falseTar));
        }
    }

    void ASTCodeGen::visit(FE::AST::CallExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 生成函数调用 IR（准备参数、可选返回寄存器、发出call）

        // TODO("Lab3-2: Implement CallExpr IR generation");
        std::string fname = node.func->getName();

        // 先拿到函数声明（形参类型）
        std::vector<FE::AST::ParamDeclarator*>* params = nullptr;
        auto itDecl = funcDecls.find(node.func);
        if (itDecl != funcDecls.end())
            params = itDecl->second->params;

        CallInst::argList args;

        size_t argc = node.args ? node.args->size() : 0;
        for (size_t i = 0; i < argc; ++i)
        {
            FE::AST::ExprNode* arg = (*node.args)[i];

            FE::AST::ParamDeclarator* pDecl = nullptr;
            if (params && i < params->size())
                pDecl = (*params)[i];

            bool isPtrParam = false;
            if (pDecl)
            {
                // 只要是数组形参，就当成指针
                if (pDecl->dims && !pDecl->dims->empty())
                    isPtrParam = true;
                else if (pDecl->type && pDecl->type->getTypeGroup() == FE::AST::TypeGroup::POINTER)
                    isPtrParam = true;   // 显式 ptr 也算
            }

            if (isPtrParam)
            {
                auto* lv = dynamic_cast<FE::AST::LeftValExpr*>(arg);
                ASSERT(lv && "数组/指针形参对应的实参必须是左值");

                apply(*this, *lv, m);   // 填好 lval2ptr
                auto itPtr = lval2ptr.find(lv);
                ASSERT(itPtr != lval2ptr.end());
                Operand* ptr = itPtr->second;

                args.push_back({DataType::PTR, ptr});
            }
            else
            {
                apply(*this, *arg, m);
                size_t areg = queryExprReg(arg);
                DataType at = queryExprType(arg);
                if (at == DataType::I1)
                {
                    areg = castTo(DataType::I1, DataType::I32, areg);
                    at   = DataType::I32;
                }
                args.push_back({at, getRegOperand(areg)});
            }
        }

        // 返回类型：优先从 funcDecls 获取
        DataType rt = DataType::I32;
        auto it = funcDecls.find(node.func);
        if (it != funcDecls.end())
        {
            rt = convert(it->second->retType);
            if (rt == DataType::I1) rt = DataType::I32;
        }
        else
        {
            // 库函数兜底（最小集合）
            if (fname == "putint" || fname == "putch" || fname == "putarray" ||
                fname == "putfloat" || fname == "putfarray" ||
                fname == "_sysy_starttime" || fname == "_sysy_stoptime" ||
                fname == "llvm.memset.p0.i32")
                rt = DataType::VOID;
            else if (fname == "getfloat")
                rt = DataType::F32;
            else
                rt = DataType::I32;
        }

        if (rt == DataType::VOID)
        {
            insert(createCallInst(DataType::VOID, fname, args));
            return;
        }

        size_t resReg = getNewRegId();
        insert(createCallInst(rt, fname, args, resReg));
        recordExprResult(&node, resReg, rt);
    }

    void ASTCodeGen::visit(FE::AST::CommaExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 依序生成逗号表达式每个子表达式的 IR

        // TODO("Lab3-2: Implement CommaExpr IR generation");
        ASSERT(node.exprs && "CommaExpr exprs 为空");

        FE::AST::ExprNode* last = nullptr;
        for (auto* e : *node.exprs)
        {
            if (!e) continue;
            last = e;
            apply(*this, *e, m);
        }

        if (last)
            recordExprResult(&node, queryExprReg(last), queryExprType(last));
    }
}  // namespace ME
