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

        // 构造 FuncDefInst：参数寄存器列表
        FuncDefInst::argList argRegs;
        paramPtrTab.clear();

        if (node.params)
        {
            for (size_t i = 0; i < node.params->size(); ++i)
            {
                auto* p = (*node.params)[i];
                DataType pt = convert(p->type);
                bool isPtr = (pt == DataType::PTR);   // 基础要求下你可以不允许 ptr 参数；但库函数会用 PTR
                paramPtrTab[i] = isPtr;

                if (pt == DataType::I1) pt = DataType::I32;
                if (!isPtr) ASSERT(pt == DataType::I32 && "Base requirement: only int/bool params");
                size_t preg = getNewRegId();
                argRegs.push_back({pt, getRegOperand(preg)});
            }
        }
            
        // 把参数列表装回 fdef
        fdef->argRegs = std::move(argRegs);

        // 参数映射：为每个参数 alloca，然后 store 传入值，符号表 entry -> ptrReg
        if (node.params)
        {
            for (size_t i = 0; i < node.params->size(); ++i)
            {
                auto* p = (*node.params)[i];
                ASSERT(p && p->entry);

                // 基础要求：不做数组维度；但允许库函数/指针参数存在（存在就当成 ptr，直接映射）
                DataType pt = convert(p->type);
                if (pt == DataType::I1) pt = DataType::I32;

                if (pt == DataType::PTR)
                {
                    // 直接把参数寄存器当作“地址”保存（不 alloca）
                    size_t incomingReg = fdef->argRegs[i].second->getRegNum();
                    name2reg.addSymbol(p->entry, incomingReg);
                    continue;
                }

                size_t ptrReg = getNewRegId();
                insert(createAllocaInst(DataType::I32, ptrReg));

                size_t incomingReg = fdef->argRegs[i].second->getRegNum();
                insert(createStoreInst(DataType::I32, incomingReg, getRegOperand(ptrReg)));

                name2reg.addSymbol(p->entry, ptrReg);
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
        if (!node.retExpr)
        {
            insert(createRetInst());
            return;
        }

        apply(*this, *node.retExpr, m);
        size_t r = getMaxReg();
        DataType t = convert(node.retExpr->attr.val.value.type);

        if (rt == DataType::VOID)
        {
            insert(createRetInst());
            return;
        }

        // 基础要求：返回 i32；bool -> i32
        if (t == DataType::I1)
        {
            auto conv = createTypeConvertInst(DataType::I1, DataType::I32, r);
            for (auto* inst : conv) insert(inst);
            r = getMaxReg();
            t = DataType::I32;
        }
        ASSERT(rt == DataType::I32 && t == DataType::I32);

        insert(createRetInst(DataType::I32, r));
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
            size_t condReg = getMaxReg();
            DataType ct = convert(node.cond->attr.val.value.type);
            if (ct == DataType::I32)
            {
                auto conv = createTypeConvertInst(DataType::I32, DataType::I1, condReg);
                for (auto* inst : conv) insert(inst);
                condReg = getMaxReg();
            }
            insert(createBranchInst(condReg, bodyL, endL));
        }

        // body
        enterBlock(bodyB);
        if (node.body)
            apply(*this, *node.body, m);
        if (bodyB->insts.empty() || !bodyB->insts.back()->isTerminator())
            bodyB->insert(createBranchInst(condL));

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
            size_t condReg = getMaxReg();
            DataType ct = convert(node.cond->attr.val.value.type);
            if (ct == DataType::I32)
            {
                auto conv = createTypeConvertInst(DataType::I32, DataType::I1, condReg);
                for (auto* inst : conv) insert(inst);
                condReg = getMaxReg();
            }
            insert(createBranchInst(condReg, thenL, elseL));
        }

        // then
        enterBlock(thenB);
        if (node.thenStmt)
            apply(*this, *node.thenStmt, m);
        if (thenB->insts.empty() || !thenB->insts.back()->isTerminator())
            thenB->insert(createBranchInst(endL));

        // else
        if (elseB)
        {
            enterBlock(elseB);
            apply(*this, *node.elseStmt, m);
            if (elseB->insts.empty() || !elseB->insts.back()->isTerminator())
                elseB->insert(createBranchInst(endL));
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
                size_t condReg = getMaxReg();
                DataType ct = convert(node.cond->attr.val.value.type);
                if (ct == DataType::I32)
                {
                    auto conv = createTypeConvertInst(DataType::I32, DataType::I1, condReg);
                    for (auto* inst : conv) insert(inst);
                    condReg = getMaxReg();
                }
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
        if (bodyB->insts.empty() || !bodyB->insts.back()->isTerminator())
            bodyB->insert(createBranchInst(stepL));

        // step
        enterBlock(stepB);
        if (node.step) apply(*this, *node.step, m);
        if (stepB->insts.empty() || !stepB->insts.back()->isTerminator())
            stepB->insert(createBranchInst(condL));

        curFunc->loopStartLabel = savedStart;
        curFunc->loopEndLabel   = savedEnd;

        // end
        enterBlock(endB);
    }
}  // namespace ME
