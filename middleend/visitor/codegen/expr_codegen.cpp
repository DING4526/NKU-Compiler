#include <middleend/visitor/codegen/ast_codegen.h>

namespace ME
{
void ASTCodeGen::visit(FE::AST::LeftValExpr& node, Module* m)
{
    (void)m;

    // 1) 找到 base ptr：局部优先，其次全局
    size_t  localPtrReg = name2reg.getReg(node.entry);
    Operand* basePtr    = nullptr;
    bool    isLocal     = (localPtrReg != static_cast<size_t>(-1));
    if (isLocal)
        basePtr = getRegOperand(localPtrReg);                     // alloca 返回的指针
    else
        basePtr = getGlobalOperand(node.entry->getName());        // 全局符号

    // 2) 取维度信息（决定是真数组还是“指针风格”）
    std::vector<int> dims;
    bool             hasDims = false;  // 真数组才会有维度
    if (isLocal)
    {
        auto ita = reg2attr.find(localPtrReg);
        if (ita != reg2attr.end() && !ita->second.arrayDims.empty())
        {
            dims    = ita->second.arrayDims;
            hasDims = true;
        }
    }
    else
    {
        // ★ 不再直接用 glbSymbols，而是用我们在 handleGlobalVarDecl 里记录的 glbArrayDims
        auto itg = glbArrayDims.find(node.entry);
        if (itg != glbArrayDims.end())
        {
            dims    = itg->second;
            hasDims = true;
        }
    }
    //     // ☆ 加一行调试：
    // printf("LeftVal %s: isLocal=%d hasDims=%d dimsSize=%zu\n",
    //        node.entry->getName().c_str(), (int)isLocal, (int)hasDims, dims.size());

    Operand* elemPtr = basePtr;

    // 3) 如果有下标 => 生成 GEP
    if (node.indices && !node.indices->empty())
    {
        std::vector<Operand*> idxOps;
        std::vector<size_t>   idxRegs;
        idxOps.reserve(node.indices->size());
        idxRegs.reserve(node.indices->size());
        for (auto* idxExpr : *node.indices)
        {
            apply(*this, *idxExpr, m);
            size_t   ir = queryExprReg(idxExpr);
            DataType it = queryExprType(idxExpr);

            // 下标统一转 i32
            if (it != DataType::I32)
                ir = castTo(it, DataType::I32, ir);

            idxRegs.push_back(ir);
            idxOps.push_back(getRegOperand(ir));
        }

        size_t nIdx = idxRegs.size();

        // ===== 真数组：使用维度信息做扁平化索引 =====
        if (hasDims)
        {
            ASSERT(nIdx <= dims.size() && "多维数组下标数超过声明维度");

            // stride[k] = dims[k+1]*dims[k+2]*...；最后一维 stride = 1
            std::vector<int> stride(dims.size());
            int acc = 1;
            for (int k = static_cast<int>(dims.size()) - 1; k >= 0; --k)
            {
                ASSERT(dims[k] > 0 && "数组维度必须为正");
                stride[k] = acc;
                acc *= dims[k];
            }

            // offset = sum_{t=0..nIdx-1} idx[t] * stride[t]
            size_t offsetReg = getNewRegId();
            // offset = 0
            insert(createArithmeticI32Inst_ImmeAll(Operator::ADD, 0, 0, offsetReg));

            for (size_t t = 0; t < nIdx; ++t)
            {
                int s = stride[t];

                size_t mulReg;
                if (s == 1)
                {
                    mulReg = idxRegs[t];
                }
                else
                {
                    mulReg = getNewRegId();
                    // 如果你已经有 _ImmeLeft 版本，这里建议用：
                    // insert(createArithmeticI32Inst_ImmeLeft(Operator::MUL, s, idxRegs[t], mulReg));
                    insert(createArithmeticI32Inst_ImmeLeft(Operator::MUL, s, idxRegs[t], mulReg));
                }

                size_t newOffset = getNewRegId();
                insert(createArithmeticI32Inst(Operator::ADD, offsetReg, mulReg, newOffset));
                offsetReg = newOffset;
            }

            size_t   gepReg = getNewRegId();
            DataType elemTy = convert(node.attr.val.value.type);
            if (elemTy == DataType::I1) elemTy = DataType::I32;

            // 这里 dims 不再参与 GEP 的类型构造（类型已经在 GlbVarDeclInst 里确定好了）
            insert(createGEP_I32Inst(
                elemTy,
                basePtr,
                /*dims*/ {},
                {getRegOperand(offsetReg)},
                gepReg));

            elemPtr = getRegOperand(gepReg);
        }
        // ===== 指针/形参数组：只允许一维下标 =====
else
{
    if (nIdx == 1)
    {
        size_t gepReg = getNewRegId();
        DataType elemTy = convert(node.attr.val.value.type);
        if (elemTy == DataType::I1) elemTy = DataType::I32;

        insert(createGEP_I32Inst(elemTy, basePtr, {}, idxOps, gepReg));
        elemPtr = getRegOperand(gepReg);
    }
    else
    {
        // 形如 b[][59]：nIdx=2，但我们知道“后续维度”= {59}
        auto itp = paramArrayDims.find(node.entry);
        ASSERT(itp != paramArrayDims.end() && "多维下标访问需要形参数组维度信息");
        const auto& inner = itp->second;
        ASSERT(inner.size() == nIdx - 1 && "形参数组维度与下标数量不匹配");

        // suffix product: suf[t] = product(inner[t..])
        std::vector<int64_t> suf(inner.size() + 1, 1);
        for (int k = (int)inner.size() - 1; k >= 0; --k)
            suf[k] = suf[k + 1] * inner[k];

        // offset = idx0*suf[0] + idx1*suf[1] + ... + idx_{nIdx-2}*suf[nIdx-2] + idx_{nIdx-1}
        size_t offsetReg = getNewRegId();
        insert(createArithmeticI32Inst_ImmeAll(Operator::ADD, 0, 0, offsetReg));

        for (size_t t = 0; t < nIdx; ++t)
        {
            size_t termReg = idxRegs[t];

            if (t < nIdx - 1)
            {
                int64_t s = suf[t];
                if (s != 1)
                {
                    size_t mulReg = getNewRegId();
                    insert(createArithmeticI32Inst_ImmeLeft(Operator::MUL, (int)s, idxRegs[t], mulReg));
                    termReg = mulReg;
                }
            }

            size_t newOffset = getNewRegId();
            insert(createArithmeticI32Inst(Operator::ADD, offsetReg, termReg, newOffset));
            offsetReg = newOffset;
        }

        size_t gepReg = getNewRegId();
        DataType elemTy = convert(node.attr.val.value.type);
        if (elemTy == DataType::I1) elemTy = DataType::I32;

        insert(createGEP_I32Inst(elemTy, basePtr, {}, { getRegOperand(offsetReg) }, gepReg));
        elemPtr = getRegOperand(gepReg);
    }
}

    }

    // 4) 不管怎样，记录“这个左值的地址”
    lval2ptr[&node] = elemPtr;

    // 5) 表达式值：统一生成 load（指针值的情况除外）
    DataType valTy = convert(node.attr.val.value.type);
    if (valTy == DataType::I1) valTy = DataType::I32;

    // 数组形参/指针本身作为值：直接返回地址寄存器（局部指针）
    if (valTy == DataType::PTR && isLocal)
    {
        recordExprResult(&node, localPtrReg, DataType::PTR);
        return;
    }

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

size_t ASTCodeGen::handleAssign(FE::AST::LeftValExpr& lhs, FE::AST::ExprNode& rhs, Module* m)
{
    // 1) 取 lhs 地址：标记 isLval，防止 LeftValExpr 里多做一次 load
    bool old = lhs.isLval;
    lhs.isLval = true;
    apply(*this, lhs, m);
    lhs.isLval = old;

    auto it = lval2ptr.find(&lhs);
    ASSERT(it != lval2ptr.end());
    Operand* ptr = it->second;

    // 2) 计算 rhs 的值
    apply(*this, rhs, m);
    size_t rhsReg = queryExprReg(&rhs);
    DataType rhsTy = queryExprType(&rhs);

    // 3) lhs 元素类型
    DataType lhsTy = convert(lhs.attr.val.value.type);
    if (lhsTy == DataType::I1) lhsTy = DataType::I32;

    if (rhsTy == DataType::I1) rhsTy = DataType::I32;
    if (rhsTy != lhsTy)
        rhsReg = castTo(rhsTy, lhsTy, rhsReg);

    // 4) store
    insert(createStoreInst(lhsTy, rhsReg, ptr));

    // 5) 赋值表达式的值：再 load 一次，返回寄存器号，供 BinaryExpr 记录
    size_t resReg = getNewRegId();
    insert(createLoadInst(lhsTy, ptr, resReg));
    return resReg;
}


    void ASTCodeGen::handleLogicalAnd(
    FE::AST::BinaryExpr& node, FE::AST::ExprNode& lhs, FE::AST::ExprNode& rhs, Module* m)
{
    // 目标：lhs 为真才计算 rhs
    ASSERT(node.trueTar != static_cast<size_t>(-1) && node.falseTar != static_cast<size_t>(-1));

    Block* rhsBlock = createBlock();
    size_t rhsLabel = rhsBlock->blockId;

    // lhs: true -> rhsLabel, false -> node.falseTar
    lhs.trueTar  = rhsLabel;
    lhs.falseTar = node.falseTar;
    apply(*this, lhs, m);

    // 如果 lhs 这一块最后没 terminator，则补一个 brcond
    if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()))
    {
        size_t   condReg = queryExprReg(&lhs);
        DataType t       = queryExprType(&lhs);

        if (t != DataType::I1)
            condReg = castTo(t, DataType::I1, condReg);

        insert(createBranchInst(condReg, rhsLabel, node.falseTar));
    }

    // 进入 rhs block：true -> node.trueTar, false -> node.falseTar
    enterBlock(rhsBlock);
    rhs.trueTar  = node.trueTar;
    rhs.falseTar = node.falseTar;
    apply(*this, rhs, m);

    if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()))
    {
        size_t   condReg = queryExprReg(&rhs);
        DataType t       = queryExprType(&rhs);

        if (t != DataType::I1)
            condReg = castTo(t, DataType::I1, condReg);

        insert(createBranchInst(condReg, node.trueTar, node.falseTar));
    }
}


void ASTCodeGen::handleLogicalOr(
    FE::AST::BinaryExpr& node, FE::AST::ExprNode& lhs, FE::AST::ExprNode& rhs, Module* m)
{
    // 目标：lhs 为假才计算 rhs
    ASSERT(node.trueTar != static_cast<size_t>(-1) && node.falseTar != static_cast<size_t>(-1));

    Block* rhsBlock = createBlock();
    size_t rhsLabel = rhsBlock->blockId;

    // lhs: true -> node.trueTar, false -> rhsLabel
    lhs.trueTar  = node.trueTar;
    lhs.falseTar = rhsLabel;
    apply(*this, lhs, m);

    if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()))
    {
        size_t   condReg = queryExprReg(&lhs);
        DataType t       = queryExprType(&lhs);

        if (t != DataType::I1)
            condReg = castTo(t, DataType::I1, condReg);

        insert(createBranchInst(condReg, node.trueTar, rhsLabel));
    }

    // rhs: true -> node.trueTar, false -> node.falseTar
    enterBlock(rhsBlock);
    rhs.trueTar  = node.trueTar;
    rhs.falseTar = node.falseTar;
    apply(*this, rhs, m);

    if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()))
    {
        size_t   condReg = queryExprReg(&rhs);
        DataType t       = queryExprType(&rhs);

        if (t != DataType::I1)
            condReg = castTo(t, DataType::I1, condReg);

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

    DataType lhsTy = convert(l->attr.val.value.type);
    if (lhsTy == DataType::I1) lhsTy = DataType::I32;

    // 生成赋值 IR，并返回赋值表达式的值所在寄存器
    size_t valReg = handleAssign(*l, *node.rhs, m);
    recordExprResult(&node, valReg, lhsTy);

    // 如果赋值表达式用于条件上下文，则再生成 brcond
    if (node.trueTar != static_cast<size_t>(-1) &&
        node.falseTar != static_cast<size_t>(-1))
    {
        size_t condReg = valReg;
        DataType ct = lhsTy;

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
