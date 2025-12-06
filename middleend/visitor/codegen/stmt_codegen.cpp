#include <middleend/visitor/codegen/ast_codegen.h>

namespace ME
{
    void ASTCodeGen::visit(FE::AST::ExprStmt& node, Module* m)
    {
        if (!node.expr) return;
        apply(*this, *node.expr, m);
    }

    void ASTCodeGen::visit(FE::AST::FuncDeclStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成函数定义 IR（形参、入口/结束基本块、返回补丁）
        // 设置函数返回类型与参数寄存器，创建基本块骨架，并生成函数体

        // TODO("Lab3-2: Implement FuncDeclStmt IR generation");
        // 基础要求：只支持返回 void/int/bool（bool 按 i32 处理），参数只支持 int/bool
        DataType rt = convert(node.retType);
        if (rt == DataType::I1) rt = DataType::I32;
        ASSERT(rt == DataType::VOID || rt == DataType::I32);

        // 先 new FuncDef / Function
        auto* fdef = new FuncDefInst(rt, node.entry->getName());
        auto* func = new Function(fdef);
        m->functions.push_back(func);

        // 进入函数：创建入口 block
        enterFunc(func);
        Block* entry = createBlock();
        enterBlock(entry);

        // 新作用域（函数级）
        name2reg.enterScope();

        // 构造参数寄存器列表
        FuncDefInst::argList argRegs;
        paramPtrTab.clear();   // 既然成员还在，就顺手用一下

        if (node.params)
        {
            for (size_t i = 0; i < node.params->size(); ++i)
            {
                auto* p = (*node.params)[i];

                // 是否为数组形参：有 dims 就当“数组 → 指针”
                bool isArrayParam = (p->dims && !p->dims->empty());

                DataType pt;
                if (isArrayParam)
                {
                    pt = DataType::PTR;                     // IR 视为指针
                }
                else
                {
                    pt = convert(p->type);                  // int / float / bool
                    if (pt == DataType::I1) pt = DataType::I32;
                }

                paramPtrTab[i] = isArrayParam;

                size_t preg = getNewRegId();
                argRegs.push_back({pt, getRegOperand(preg)});
            }
        }
        fdef->argRegs = std::move(argRegs);

        // 映射到符号表：区分标量参数 / 指针（数组）参数
        if (node.params)
        {
            for (size_t i = 0; i < node.params->size(); ++i)
            {
                auto* p = (*node.params)[i];
                bool isPtrParam = paramPtrTab[i];   // true => 数组形参，被视为 PTR

                size_t incomingReg = fdef->argRegs[i].second->getRegNum();

                if (isPtrParam)
                {
                    // 形参本身就是“地址”
                    name2reg.addSymbol(p->entry, incomingReg);

                    // 为数组/指针形参构造 dims，填进 reg2attr
                    std::vector<int> dims;
                    if (p->dims && !p->dims->empty())
                    {
                        dims.reserve(p->dims->size());
                        for (auto* dimExpr : *p->dims)
                        {
                            int d = -1;  // 不可求值/省略时用 -1
                            if (dimExpr && dimExpr->attr.val.isConstexpr)
                                d = dimExpr->attr.val.getInt();   // 例如 int b[4][1024]
                            dims.push_back(d);
                        }
                    }
                    else
                    {
                        // 防御性：int *p 这类也当 1 维未知长度
                        dims.push_back(-1);
                    }

                    reg2attr[incomingReg] = FE::AST::VarAttr(
                        p->type,          // AST 上的原始类型（base 是 int / float）
                        /*isConstDecl*/ false,
                        /*level*/ -1,
                        dims,
                        {}                // initList 对形参无意义
                    );
                }
                else
                {
                    // 非指针形参：在栈上 alloca 再 store，后续当局部变量用
                    DataType pt = convert(p->type);
                    if (pt == DataType::I1) pt = DataType::I32;

                    size_t ptrReg = getNewRegId();
                    insert(createAllocaInst(pt, ptrReg));
                    insert(createStoreInst(pt, incomingReg, getRegOperand(ptrReg)));

                    name2reg.addSymbol(p->entry, ptrReg);
                }
            }
        }

        // 生成函数体
        if (node.body) apply(*this, *node.body, m);

        // 退出当前块前：保证 terminator
        if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()))
        {
            if (rt == DataType::VOID)
                insert(createRetInst());
            else
                insert(createRetInst(0)); // patch: return 0
        }

        // 清理
        name2reg.exitScope();
        exitBlock();
        exitFunc();
    }

    void ASTCodeGen::visit(FE::AST::VarDeclStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成变量声明语句 IR（局部变量分配、初始化）

        // TODO("Lab3-2: Implement VarDeclStmt IR generation");
        ASSERT(node.decl);
        apply(*this, *node.decl, m);
    }

    void ASTCodeGen::visit(FE::AST::BlockStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成语句块 IR（作用域管理，顺序生成子语句）

        // TODO("Lab3-2: Implement BlockStmt IR generation");
        name2reg.enterScope();

        if (node.stmts)
        {
            for (auto* s : *node.stmts)
            {
                if (!s) continue;

                // 如果当前块已经终结，后面的语句即使生成也是不可达；基础要求下可直接跳过
                if (curBlock && !curBlock->insts.empty() && curBlock->insts.back()->isTerminator())
                    break;

                apply(*this, *s, m);
            }
        }

        name2reg.exitScope();
    }

    void ASTCodeGen::visit(FE::AST::ReturnStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 return 语句 IR（可选返回值与类型转换）

        // TODO("Lab3-2: Implement ReturnStmt IR generation");
        (void)m;

        DataType rt = curFunc->funcDef->retType;
        if (rt == DataType::I1) rt = DataType::I32; // 保持你原有约定

        if (!node.retExpr)
        {
            insert(createRetInst());
            return;
        }

        apply(*this, *node.retExpr, m);

        // ★ 禁止 getMaxReg 猜：必须从表里取
        size_t r  = queryExprReg(node.retExpr);
        DataType t = queryExprType(node.retExpr);

        if (t == DataType::I1) { r = castTo(DataType::I1, DataType::I32, r); t = DataType::I32; }

        if (rt == DataType::VOID)
        {
            insert(createRetInst());
            return;
        }

        if (t != rt)
            r = castTo(t, rt, r);

        insert(createRetInst(rt, r));
    }

    void ASTCodeGen::visit(FE::AST::WhileStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 while 循环 IR（条件块、循环体与结束块、循环标签）

        // TODO("Lab3-2: Implement WhileStmt IR generation");
        // 允许 while(cond); 这种空循环体：body 可以为 nullptr
        ASSERT(node.cond);

        Block* condB = createBlock();
        Block* bodyB = createBlock();
        Block* endB  = createBlock();

        size_t condL = condB->blockId;
        size_t bodyL = bodyB->blockId;
        size_t endL  = endB->blockId;

        // 从当前块跳到 cond
        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())
            insert(createBranchInst(condL));

        // 设置循环标签（用于 break/continue）
        size_t savedStart = curFunc->loopStartLabel;
        size_t savedEnd   = curFunc->loopEndLabel;
        curFunc->loopStartLabel = condL;
        curFunc->loopEndLabel   = endL;

        // cond
        enterBlock(condB);
        node.cond->trueTar  = bodyL;
        node.cond->falseTar = endL;
        apply(*this, *node.cond, m);

        if (condB->insts.empty() || !condB->insts.back()->isTerminator())
        {
            size_t condReg = queryExprReg(node.cond);
            DataType ct = queryExprType(node.cond);
            if (ct != DataType::I1) condReg = castTo(ct, DataType::I1, condReg);

            insert(createBranchInst(condReg, bodyL, endL));
        }

        // body
        enterBlock(bodyB);
        if (node.body)
            apply(*this, *node.body, m);
        if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()))
            insert(createBranchInst(condL));

        // restore loop labels
        curFunc->loopStartLabel = savedStart;
        curFunc->loopEndLabel   = savedEnd;

        // end
        enterBlock(endB);
    }

    void ASTCodeGen::visit(FE::AST::IfStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 if/else IR（then/else/end 基本块与条件分支）

        // TODO("Lab3-2: Implement IfStmt IR generation");
        ASSERT(node.cond);

        Block* thenB = createBlock();
        Block* endB  = createBlock();
        Block* elseB = node.elseStmt ? createBlock() : nullptr;

        size_t thenL = thenB->blockId;
        size_t elseL = elseB ? elseB->blockId : endB->blockId;
        size_t endL  = endB->blockId;

        // cond：设置跳转目标，并生成（可能产生 brcond，也可能是 &&/|| 链）
        node.cond->trueTar  = thenL;
        node.cond->falseTar = elseL;
        apply(*this, *node.cond, m);

        // 如果 cond 最后没发 terminator（例如是字面量/变量），这里补一个 brcond
        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())
        {
            size_t condReg = queryExprReg(node.cond);
            DataType ct    = queryExprType(node.cond);

            if (ct != DataType::I1)
                condReg = castTo(ct, DataType::I1, condReg);

            insert(createBranchInst(condReg, thenL, elseL));
        }

        // then
        enterBlock(thenB);
        if (node.thenStmt)
            apply(*this, *node.thenStmt, m);
        if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()))
            insert(createBranchInst(endL));

        // else
        if (elseB)
        {
            enterBlock(elseB);
            if (node.elseStmt)
                apply(*this, *node.elseStmt, m);
            if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()))
                insert(createBranchInst(endL));
        }

        // end
        enterBlock(endB);
    }

    void ASTCodeGen::visit(FE::AST::BreakStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 break 的无条件跳转至循环结束块

        // TODO("Lab3-2: Implement BreakStmt IR generation");
        (void)node; (void)m;
        ASSERT(curFunc && curFunc->loopEndLabel != 0 && "break not in loop");
        insert(createBranchInst(curFunc->loopEndLabel));
    }

    void ASTCodeGen::visit(FE::AST::ContinueStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 continue 的无条件跳转至循环步进/条件块

        // TODO("Lab3-2: Implement ContinueStmt IR generation");
        (void)node; (void)m;
        ASSERT(curFunc && curFunc->loopStartLabel != 0 && "continue not in loop");
        insert(createBranchInst(curFunc->loopStartLabel));
    }

    void ASTCodeGen::visit(FE::AST::ForStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 for 循环 IR（init/cond/body/step 基本块与循环标签）

        // TODO("Lab3-2: Implement ForStmt IR generation");
        // 最小实现：for(init;cond;step) body
        // 展开为：init; while(cond){ body; step; }
        if (node.init) apply(*this, *node.init, m);

        // blocks
        Block* condB = createBlock();
        Block* bodyB = createBlock();
        Block* stepB = createBlock();
        Block* endB  = createBlock();

        size_t condL = condB->blockId;
        size_t bodyL = bodyB->blockId;
        size_t stepL = stepB->blockId;
        size_t endL  = endB->blockId;

        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())
            insert(createBranchInst(condL));

        size_t savedStart = curFunc->loopStartLabel;
        size_t savedEnd   = curFunc->loopEndLabel;
        curFunc->loopStartLabel = stepL; // continue -> step
        curFunc->loopEndLabel   = endL;

        // cond
        enterBlock(condB);
        if (node.cond)
        {
            node.cond->trueTar  = bodyL;
            node.cond->falseTar = endL;
            apply(*this, *node.cond, m);

            if (condB->insts.empty() || !condB->insts.back()->isTerminator())
            {
                size_t condReg = queryExprReg(node.cond);
                DataType ct = queryExprType(node.cond);
                if (ct != DataType::I1) condReg = castTo(ct, DataType::I1, condReg);

                insert(createBranchInst(condReg, bodyL, endL));
            }
        }
        else
        {
            // cond 缺省视为 true
            insert(createBranchInst(bodyL));
        }

        // body
        enterBlock(bodyB);
        if (node.body) apply(*this, *node.body, m);
        if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()))
            insert(createBranchInst(stepL));

        // step
        enterBlock(stepB);
        if (node.step) apply(*this, *node.step, m);
        if (curBlock && (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()))
            insert(createBranchInst(condL));

        curFunc->loopStartLabel = savedStart;
        curFunc->loopEndLabel   = savedEnd;

        // end
        enterBlock(endB);
    }
}  // namespace ME
