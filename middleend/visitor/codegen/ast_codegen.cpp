#include <middleend/visitor/codegen/ast_codegen.h>
#include <debug.h>

namespace ME
{
    void ASTCodeGen::libFuncRegister(Module* m)
    {
        auto& decls = m->funcDecls;

        // int getint();
        decls.emplace_back(new FuncDeclInst(DataType::I32, "getint"));

        // int getch();
        decls.emplace_back(new FuncDeclInst(DataType::I32, "getch"));

        // int getarray(int a[]);
        decls.emplace_back(new FuncDeclInst(DataType::I32, "getarray", {DataType::PTR}));

        // float getfloat();
        decls.emplace_back(new FuncDeclInst(DataType::F32, "getfloat"));

        // int getfarray(float a[]);
        decls.emplace_back(new FuncDeclInst(DataType::I32, "getfarray", {DataType::PTR}));

        // void putint(int a);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "putint", {DataType::I32}));

        // void putch(int a);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "putch", {DataType::I32}));

        // void putarray(int n, int a[]);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "putarray", {DataType::I32, DataType::PTR}));

        // void putfloat(float a);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "putfloat", {DataType::F32}));

        // void putfarray(int n, float a[]);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "putfarray", {DataType::I32, DataType::PTR}));

        // void starttime(int lineno);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "_sysy_starttime", {DataType::I32}));

        // void stoptime(int lineno);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "_sysy_stoptime", {DataType::I32}));

        // llvm memset
        decls.emplace_back(new FuncDeclInst(
            DataType::VOID, "llvm.memset.p0.i32", {DataType::PTR, DataType::I8, DataType::I32, DataType::I1}));
    }

    void ASTCodeGen::handleGlobalVarDecl(FE::AST::VarDeclStmt* decls, Module* m)
    {
        ASSERT(decls && decls->decl);
        auto* vd = decls->decl;

        DataType base = convert(vd->type);
        if (base == DataType::I1) base = DataType::I32;
        ASSERT(base == DataType::I32 || base == DataType::F32);

        for (auto* d : *vd->decls)
        {
            auto* lv = dynamic_cast<FE::AST::LeftValExpr*>(d->lval);
            ASSERT(lv);

            // 从符号表拷贝一份 VarAttr，方便我们在本函数里修改
            auto itSym = glbSymbols.find(lv->entry);
            ASSERT(itSym != glbSymbols.end());
            FE::AST::VarAttr attr = itSym->second;

            std::string name = lv->entry->getName();

            // ===== 1. 先判断是不是“数组声明” =====
            bool isArrayByAst = (lv->indices && !lv->indices->empty());
            std::vector<int> dims;

            if (isArrayByAst)
            {
                // 优先用语义阶段算好的 arrayDims
                if (!attr.arrayDims.empty())
                {
                    dims = attr.arrayDims;
                }
                else
                {
                    // 退化方案：从 indices 的常量表达式里算维度
                    for (auto* dimExpr : *lv->indices)
                    {
                        // 全局数组维度必须是编译期常量
                        ASSERT(dimExpr->attr.val.isConstexpr && "global array dim must be constexpr");
                        int dval = dimExpr->attr.val.getInt();
                        ASSERT(dval > 0 && "array dimension must be positive");
                        dims.push_back(dval);
                    }
                    attr.arrayDims = dims;  // 补回去，后面 GlbVarDeclInst 也会用到
                }

//                     std::cerr << "[GVAR] name=" << name
//               << " dims=[";
//     for (size_t i = 0; i < dims.size(); ++i)
//         std::cerr << dims[i] << (i + 1 < dims.size() ? "," : "");
//     std::cerr << "]";

//     // 关键：打印 attr 里和“初始化”有关的东西
//     // 你工程里 VarAttr 一般会有类似：
//     //   std::vector<ExprValue> initList;
//     // 或者 flattenInits / initVals ... 自己对一下名字即可
//     std::cerr << "  hasInitAst=" << (d->init != nullptr);
//     std::cerr << "  attr.arrayDims.size=" << attr.arrayDims.size();
//     // 如果有 attr.initList
//     std::cerr << "  attr.initList.size=" << attr.initList.size();
//     std::cerr << std::endl;

// if (!attr.initList.empty()) {
//     std::cerr << "  first 8 inits:\n";
//     for (size_t k = 0; k < std::min<size_t>(8, attr.initList.size()); ++k) {
//         auto &vv = attr.initList[k];
//         std::cerr << "    [" << k << "] type=" 
//                   << (vv.type ? vv.type->toString() : std::string("null"));

//         if (vv.type == FE::AST::intType ||
//             vv.type == FE::AST::boolType ||
//             vv.type == FE::AST::llType ||
//             vv.type == FE::AST::floatType) {
//             std::cerr << ", asInt=" << vv.getInt();
//         }
//         std::cerr << "\n";
//     }
// }

                // 记录到我们自己的表里，供 LeftValExpr 使用
                glbArrayDims[lv->entry] = dims;

                // 数组：用 VarAttr 版本（里边有 baseType + dims + 扁平 initList）
                m->globalVars.emplace_back(new GlbVarDeclInst(base, name, attr));
                continue;
            }

            // ===== 2. 不是数组 => 标量全局变量 =====
            int   ival    = 0;
            float fval    = 0.0f;
            bool  hasInit = (d->init != nullptr);

            if (hasInit)
            {
                ASSERT(d->init->singleInit);
                auto* init = dynamic_cast<FE::AST::Initializer*>(d->init);
                ASSERT(init && init->init_val);

                auto& ev = init->init_val->attr.val;
                ASSERT(ev.isConstexpr);

                if (base == DataType::I32)
                    ival = ev.getInt();
                else
                    fval = ev.getFloat();
            }

            Operand* initOp = (base == DataType::I32)
                                ? (Operand*)getImmeI32Operand(ival)
                                : (Operand*)getImmeF32Operand(fval);
            m->globalVars.emplace_back(new GlbVarDeclInst(base, name, initOp));
        }
    }

    void ASTCodeGen::visit(FE::AST::Root& node, Module* m)
    {
        // 示例：注册库函数
        libFuncRegister(m);

        // TODO(Lab 3-2): 生成模块级 IR
        // 处理顶层语句：全局变量声明、函数定义等

        // TODO("Lab3-2: Implement Root IR generation");
        for (auto* stmt : *node.getStmts())
        {
            if (!stmt) continue;

            switch (stmt->getStmtType())
            {
                case FE::AST::StmtType::VARDECLSTMT:
                {
                    auto* vds = dynamic_cast<FE::AST::VarDeclStmt*>(stmt);
                    handleGlobalVarDecl(vds, m);
                    break;
                }
                case FE::AST::StmtType::FUNCDECLSTMT:
                {
                    apply(*this, *stmt, m);
                    break;
                }
                default:
                    ERROR("Unsupported top-level stmt type in Root");
            }
        }
    }

    LoadInst* ASTCodeGen::createLoadInst(DataType t, Operand* ptr, size_t resReg)
    {
        return new LoadInst(t, ptr, getRegOperand(resReg));
    }

    StoreInst* ASTCodeGen::createStoreInst(DataType t, size_t valReg, Operand* ptr)
    {
        return new StoreInst(t, getRegOperand(valReg), ptr);
    }
    StoreInst* ASTCodeGen::createStoreInst(DataType t, Operand* val, Operand* ptr)
    {
        return new StoreInst(t, val, ptr);
    }

    ArithmeticInst* ASTCodeGen::createArithmeticI32Inst(Operator op, size_t lhsReg, size_t rhsReg, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::I32, getRegOperand(lhsReg), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    ArithmeticInst* ASTCodeGen::createArithmeticI32Inst_ImmeLeft(Operator op, int lhsVal, size_t rhsReg, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::I32, getImmeI32Operand(lhsVal), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    ArithmeticInst* ASTCodeGen::createArithmeticI32Inst_ImmeAll(Operator op, int lhsVal, int rhsVal, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::I32, getImmeI32Operand(lhsVal), getImmeI32Operand(rhsVal), getRegOperand(resReg));
    }
    ArithmeticInst* ASTCodeGen::createArithmeticF32Inst(Operator op, size_t lhsReg, size_t rhsReg, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::F32, getRegOperand(lhsReg), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    ArithmeticInst* ASTCodeGen::createArithmeticF32Inst_ImmeLeft(
        Operator op, float lhsVal, size_t rhsReg, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::F32, getImmeF32Operand(lhsVal), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    ArithmeticInst* ASTCodeGen::createArithmeticF32Inst_ImmeAll(Operator op, float lhsVal, float rhsVal, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::F32, getImmeF32Operand(lhsVal), getImmeF32Operand(rhsVal), getRegOperand(resReg));
    }

    IcmpInst* ASTCodeGen::createIcmpInst(ICmpOp cond, size_t lhsReg, size_t rhsReg, size_t resReg)
    {
        return new IcmpInst(DataType::I32, cond, getRegOperand(lhsReg), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    IcmpInst* ASTCodeGen::createIcmpInst_ImmeRight(ICmpOp cond, size_t lhsReg, int rhsVal, size_t resReg)
    {
        return new IcmpInst(
            DataType::I32, cond, getRegOperand(lhsReg), getImmeI32Operand(rhsVal), getRegOperand(resReg));
    }
    FcmpInst* ASTCodeGen::createFcmpInst(FCmpOp cond, size_t lhsReg, size_t rhsReg, size_t resReg)
    {
        return new FcmpInst(DataType::F32, cond, getRegOperand(lhsReg), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    FcmpInst* ASTCodeGen::createFcmpInst_ImmeRight(FCmpOp cond, size_t lhsReg, float rhsVal, size_t resReg)
    {
        return new FcmpInst(
            DataType::F32, cond, getRegOperand(lhsReg), getImmeF32Operand(rhsVal), getRegOperand(resReg));
    }

    FP2SIInst* ASTCodeGen::createFP2SIInst(size_t srcReg, size_t destReg)
    {
        return new FP2SIInst(getRegOperand(srcReg), getRegOperand(destReg));
    }
    SI2FPInst* ASTCodeGen::createSI2FPInst(size_t srcReg, size_t destReg)
    {
        return new SI2FPInst(getRegOperand(srcReg), getRegOperand(destReg));
    }
    ZextInst* ASTCodeGen::createZextInst(size_t srcReg, size_t destReg, size_t srcBits, size_t destBits)
    {
        ASSERT(srcBits == 1 && destBits == 32 && "Currently only support i1 to i32 zext");
        return new ZextInst(DataType::I1, DataType::I32, getRegOperand(srcReg), getRegOperand(destReg));
    }

    GEPInst* ASTCodeGen::createGEP_I32Inst(
        DataType t, Operand* ptr, std::vector<int> dims, std::vector<Operand*> is, size_t resReg)
    {
        return new GEPInst(t, DataType::I32, ptr, getRegOperand(resReg), dims, is);
    }

    CallInst* ASTCodeGen::createCallInst(DataType t, std::string funcName, CallInst::argList args, size_t resReg)
    {
        return new CallInst(t, funcName, args, getRegOperand(resReg));
    }
    CallInst* ASTCodeGen::createCallInst(DataType t, std::string funcName, CallInst::argList args)
    {
        return new CallInst(t, funcName, args);
    }
    CallInst* ASTCodeGen::createCallInst(DataType t, std::string funcName, size_t resReg)
    {
        return new CallInst(t, funcName, getRegOperand(resReg));
    }
    CallInst* ASTCodeGen::createCallInst(DataType t, std::string funcName) { return new CallInst(t, funcName); }

    RetInst* ASTCodeGen::createRetInst() { return new RetInst(DataType::VOID); }
    RetInst* ASTCodeGen::createRetInst(DataType t, size_t retReg) { return new RetInst(t, getRegOperand(retReg)); }
    RetInst* ASTCodeGen::createRetInst(int val) { return new RetInst(DataType::I32, getImmeI32Operand(val)); }
    RetInst* ASTCodeGen::createRetInst(float val) { return new RetInst(DataType::F32, getImmeF32Operand(val)); }

    BrCondInst* ASTCodeGen::createBranchInst(size_t condReg, size_t trueTar, size_t falseTar)
    {
        return new BrCondInst(getRegOperand(condReg), getLabelOperand(trueTar), getLabelOperand(falseTar));
    }
    BrUncondInst* ASTCodeGen::createBranchInst(size_t tar) { return new BrUncondInst(getLabelOperand(tar)); }

    AllocaInst* ASTCodeGen::createAllocaInst(DataType t, size_t ptrReg)
    {
        return new AllocaInst(t, getRegOperand(ptrReg));
    }
    AllocaInst* ASTCodeGen::createAllocaInst(DataType t, size_t ptrReg, std::vector<int> dims)
    {
        return new AllocaInst(t, getRegOperand(ptrReg), dims);
    }

    std::list<Instruction*> ASTCodeGen::createTypeConvertInst(DataType from, DataType to, size_t srcReg)
    {
        if (from == to) return {};
        ASSERT((from == DataType::I1) || (from == DataType::I32) || (from == DataType::F32));
        ASSERT((to == DataType::I1) || (to == DataType::I32) || (to == DataType::F32));

        std::list<Instruction*> insts;

        switch (from)
        {
            case DataType::I1:
            {
                switch (to)
                {
                    case DataType::I32:
                    {
                        size_t    destReg = getNewRegId();
                        ZextInst* zext    = createZextInst(srcReg, destReg, 1, 32);
                        insts.push_back(zext);
                        break;
                    }
                    case DataType::F32:
                    {
                        size_t    i32Reg = getNewRegId();
                        ZextInst* zext   = createZextInst(srcReg, i32Reg, 1, 32);
                        insts.push_back(zext);
                        size_t f32Reg = getNewRegId();
                        insts.push_back(createSI2FPInst(i32Reg, f32Reg));
                        break;
                    }
                    default: ERROR("Type conversion not supported");
                }
                break;
            }
            case DataType::I32:
            {
                switch (to)
                {
                    case DataType::I1:
                    {
                        size_t    destReg = getNewRegId();
                        IcmpInst* icmp    = createIcmpInst_ImmeRight(ICmpOp::NE, srcReg, 0, destReg);
                        insts.push_back(icmp);
                        break;
                    }
                    case DataType::F32:
                    {
                        size_t destReg = getNewRegId();
                        insts.push_back(createSI2FPInst(srcReg, destReg));
                        break;
                    }
                    default: ERROR("Type conversion not supported");
                }
                break;
            }
            case DataType::F32:
            {
                switch (to)
                {
                    case DataType::I1:
                    {
                        size_t    destReg = getNewRegId();
                        FcmpInst* fcmp    = createFcmpInst_ImmeRight(FCmpOp::ONE, srcReg, 0.0f, destReg);
                        insts.push_back(fcmp);
                        break;
                    }
                    case DataType::I32:
                    {
                        size_t destReg = getNewRegId();
                        insts.push_back(createFP2SIInst(srcReg, destReg));
                        break;
                    }
                    default: ERROR("Type conversion not supported");
                }
                break;
            }
            default: ERROR("Type conversion not supported");
        }

        return insts;
    }
}  // namespace ME
