/**
 * @file dag_builder.cpp
 * @brief DAGBuilder 的实现 - 将 LLVM IR 转换为 SelectionDAG
 *
 * 本文件实现了 DAGBuilder 类的所有 visit 方法，每个方法负责
 * 处理一种特定的 IR 指令，并创建相应的 DAG 节点。
 *
 * 核心设计：
 * 1. 值映射管理：通过 reg_value_map_ 维护 IR 虚拟寄存器到 DAG 节点的映射
 * 2. Chain 依赖管理：通过 currentChain_ 串联所有副作用操作
 * 3. 节点去重：所有节点创建都通过 SelectionDAG::getNode()，自动实现 CSE
 *
 * 关键方法：
 * - getValue(): 获取 IR 操作数对应的 DAG 节点
 * - setDef(): 记录 IR 指令的定义结果
 * - visit(XxxInst&): 为每种 IR 指令创建对应的 DAG 节点
 *
 * @brief 若有兴趣了解更多LLVM中DAG的具体内容，可参考
 * https://llvm.org/devmtg/2024-10/slides/tutorial/MacLean-Fargnoli-ABeginnersGuide-to-SelectionDAG.pdf
 * or
 * https://zhuanlan.zhihu.com/p/600170077
 */

#include <backend/dag/dag_builder.h>
#include <cstdint>
#include <debug.h>

namespace BE
{
    namespace DAG
    {
        static inline bool isFloatType(ME::DataType t) { return t == ME::DataType::F32 || t == ME::DataType::DOUBLE; }

        BE::DataType* DAGBuilder::mapType(ME::DataType t)
        {
            switch (t)
            {
                case ME::DataType::I1:
                case ME::DataType::I8:
                case ME::DataType::I32: return BE::I32;
                case ME::DataType::I64:
                case ME::DataType::PTR: return BE::I64;
                case ME::DataType::F32: return BE::F32;
                case ME::DataType::DOUBLE: return BE::F64;
                default: ERROR("Unsupported IR data type"); return BE::I32;
            }
        }

        void DAGBuilder::visit(ME::Module& module, SelectionDAG& dag)
        {
            for (auto* f : module.functions)
            {
                if (!f) continue;
                apply(*this, *f, dag);
            }
        }
        void DAGBuilder::visit(ME::Function& func, SelectionDAG& dag)
        {
            // 注意：当前后端 Pipeline 实际按“基本块”构建 DAG（见 BackendTarget::buildDAG）。
            // 这里提供一个最小的函数级遍历实现，便于测试或未来扩展。
            reg_value_map_.clear();
            for (auto& [_, block] : func.blocks)
            {
                if (!block) continue;
                apply(*this, *block, dag);
            }
        }

        SDValue DAGBuilder::getValue(ME::Operand* op, SelectionDAG& dag, BE::DataType* dtype)
        {
            if (!op) return SDValue();
            switch (op->getType())
            {
                case ME::OperandType::REG:
                {
                    ASSERT(dtype != nullptr && "dtype required for REG operands");

                    size_t id = op->getRegNum();
                    auto   it = reg_value_map_.find(id);
                    if (it != reg_value_map_.end()) return it->second;

                    SDValue v = dag.getRegNode(id, dtype);

                    reg_value_map_[id] = v;
                    return v;
                }
                case ME::OperandType::IMMEI32:
                {
                    auto imm = static_cast<ME::ImmeI32Operand*>(op)->value;
                    return dag.getConstantI64(imm, BE::I32);
                }
                case ME::OperandType::IMMEF32:
                {
                    auto imm = static_cast<ME::ImmeF32Operand*>(op)->value;
                    return dag.getConstantF32(imm, BE::F32);
                }
                case ME::OperandType::GLOBAL:
                {
                    // 任务：将全局符号映射为 SYMBOL 节点并返回
                    // 建议：使用 dag.getSymNode(ISD::SYMBOL, {BE::PTR}, {}, name)
                    auto* g = static_cast<ME::GlobalOperand*>(op);
                    return dag.getSymNode(static_cast<uint32_t>(ISD::SYMBOL), {BE::PTR}, {}, g->name);
                }
                case ME::OperandType::LABEL:
                {
                    // 任务：将标签操作数映射为 LABEL 节点（立即数携带标签 id）
                    // 建议：使用 dag.getImmNode(ISD::LABEL, {}, {}, labelId)
                    auto* l = static_cast<ME::LabelOperand*>(op);
                    return dag.getImmNode(static_cast<uint32_t>(ISD::LABEL), {}, {}, static_cast<int64_t>(l->lnum));
                }
                default: ERROR("Unsupported IR operand in DAGBuilder"); return SDValue();
            }
        }

        void DAGBuilder::setDef(ME::Operand* res, const SDValue& val, SelectionDAG& dag)
        {
            if (!res || res->getType() != ME::OperandType::REG) return;
            size_t regId = res->getRegNum();
            
            // 如果节点已经有 IR 寄存器 ID，且与当前不同，说明发生了 CSE
            // 需要为当前 IR 寄存器创建一个 COPY 节点，确保每个 IR 寄存器有独立的定义
            if (val.getNode() && val.getNode()->hasIRRegId() && val.getNode()->getIRRegId() != regId)
            {
                // 创建 COPY 节点：COPY 接收源值，输出到当前寄存器
                BE::DataType* vt = val.getNode()->getNumValues() > 0 ? val.getNode()->getValueType(0) : BE::I32;
                SDValue copyNode = dag.getNode(static_cast<unsigned>(ISD::COPY), {vt}, {val});
                copyNode.getNode()->setIRRegId(regId);
                reg_value_map_[regId] = copyNode;
                return;
            }
            
            reg_value_map_[regId] = val;
            if (val.getNode() && !val.getNode()->hasIRRegId()) 
                val.getNode()->setIRRegId(regId);
        }

        uint32_t DAGBuilder::mapArithmeticOpcode(ME::Operator op, bool isFloat)
        {
            if (isFloat)
            {
                switch (op)
                {
                    case ME::Operator::FADD: return static_cast<uint32_t>(ISD::FADD);
                    case ME::Operator::FSUB: return static_cast<uint32_t>(ISD::FSUB);
                    case ME::Operator::FMUL: return static_cast<uint32_t>(ISD::FMUL);
                    case ME::Operator::FDIV: return static_cast<uint32_t>(ISD::FDIV);
                    default: ERROR("Unsupported float arithmetic opcode"); return static_cast<uint32_t>(ISD::FADD);
                }
            }
            switch (op)
            {
                case ME::Operator::ADD: return static_cast<uint32_t>(ISD::ADD);
                case ME::Operator::SUB: return static_cast<uint32_t>(ISD::SUB);
                case ME::Operator::MUL: return static_cast<uint32_t>(ISD::MUL);
                case ME::Operator::DIV: return static_cast<uint32_t>(ISD::DIV);
                case ME::Operator::MOD: return static_cast<uint32_t>(ISD::MOD);
                case ME::Operator::SHL: return static_cast<uint32_t>(ISD::SHL);
                case ME::Operator::ASHR: return static_cast<uint32_t>(ISD::ASHR);
                case ME::Operator::LSHR: return static_cast<uint32_t>(ISD::LSHR);
                case ME::Operator::BITAND: return static_cast<uint32_t>(ISD::AND);
                case ME::Operator::BITXOR: return static_cast<uint32_t>(ISD::XOR);
                default: ERROR("Unsupported integer arithmetic opcode"); return static_cast<uint32_t>(ISD::ADD);
            }
        }

        void DAGBuilder::visit(ME::Block& block, SelectionDAG& dag)
        {
            currentChain_ = dag.getNode(static_cast<unsigned>(ISD::ENTRY_TOKEN), {BE::TOKEN}, {});
            for (auto* inst : block.insts) apply(*this, *inst, dag);
        }

        void DAGBuilder::visit(ME::RetInst& inst, SelectionDAG& dag)
        {
            std::vector<SDValue> ops;

            // 结合此处考虑 currentChain_ 的作用是什么
            ops.push_back(currentChain_);

            if (inst.res == nullptr)
            {
                dag.getNode(static_cast<unsigned>(ISD::RET), {}, ops);
                return;
            }

            if (inst.res->getType() == ME::OperandType::IMMEI32)
            {
                auto v = dag.getNode(static_cast<unsigned>(ISD::CONST_I32), {I32}, {});
                v.getNode()->setImmI64(static_cast<ME::ImmeI32Operand*>(inst.res)->value);
                ops.push_back(v);
            }
            else if (inst.res->getType() == ME::OperandType::IMMEF32)
            {
                auto v = dag.getNode(static_cast<unsigned>(ISD::CONST_F32), {F32}, {});
                v.getNode()->setImmF32(static_cast<ME::ImmeF32Operand*>(inst.res)->value);
                ops.push_back(v);
            }
            else if (inst.res->getType() == ME::OperandType::REG)
            {
                auto v = getValue(inst.res, dag, mapType(inst.rt));
                ops.push_back(v);
            }
            else
                ERROR("Unsupported return operand type in DAGBuilder");

            dag.getNode(static_cast<unsigned>(ISD::RET), {}, ops);
        }

        void DAGBuilder::visit(ME::LoadInst& inst, SelectionDAG& dag)
        {
            auto    vt  = mapType(inst.dt);
            SDValue ptr = getValue(inst.ptr, dag, BE::PTR);
            // LOAD: (Chain, Address) -> (Value, Chain)
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::LOAD), {vt, BE::TOKEN}, {currentChain_, ptr});
            setDef(inst.res, SDValue(node.getNode(), 0), dag);  // Value is result #0
            currentChain_ = SDValue(node.getNode(), 1);    // Chain is result #1
        }

        void DAGBuilder::visit(ME::StoreInst& inst, SelectionDAG& dag)
        {
            // 任务：生成 STORE 节点并维护 Chain
            // 要求：
            // 1) 计算 val 与 ptr 的 SDValue：val = getValue(..., mapType(inst.dt))，ptr = getValue(..., BE::PTR)
            // 2) 生成 STORE 节点：结果类型为 {TOKEN}，操作数顺序为 {currentChain_, val, ptr}
            // 3) 更新 currentChain_ = storeNode
            auto    vt  = mapType(inst.dt);
            SDValue val = getValue(inst.val, dag, vt);
            SDValue ptr = getValue(inst.ptr, dag, BE::PTR);
            SDValue st  = dag.getNode(static_cast<unsigned>(ISD::STORE), {BE::TOKEN}, {currentChain_, val, ptr});
            currentChain_ = st;
        }

        void DAGBuilder::visit(ME::ArithmeticInst& inst, SelectionDAG& dag)
        {
            bool     f    = isFloatType(inst.dt);
            auto     vt   = mapType(inst.dt);
            SDValue  lhs  = getValue(inst.lhs, dag, vt);
            SDValue  rhs  = getValue(inst.rhs, dag, vt);
            uint32_t opc  = mapArithmeticOpcode(inst.opcode, f);
            SDValue  node = dag.getNode(opc, {vt}, {lhs, rhs});
            setDef(inst.res, node, dag);
        }

        void DAGBuilder::visit(ME::IcmpInst& inst, SelectionDAG& dag)
        {
            // 任务：生成 ICMP 节点，结果类型为 I32，携带比较条件 imm
            // 提示：lhs/rhs 用 I32 作为 dtype；setImmI64(static_cast<int64_t>(inst.cond))
            SDValue lhs  = getValue(inst.lhs, dag, BE::I32);
            SDValue rhs  = getValue(inst.rhs, dag, BE::I32);
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::ICMP), {BE::I32}, {lhs, rhs});
            node.getNode()->setImmI64(static_cast<int64_t>(inst.cond));
            setDef(inst.res, node, dag);
        }

        void DAGBuilder::visit(ME::FcmpInst& inst, SelectionDAG& dag)
        {
            // 任务：生成 FCMP 节点，结果类型为 I32，携带浮点比较条件 imm
            // 提示：lhs/rhs 用 F32 作为 dtype
            auto    ft   = mapType(inst.dt);
            SDValue lhs  = getValue(inst.lhs, dag, ft);
            SDValue rhs  = getValue(inst.rhs, dag, ft);
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::FCMP), {BE::I32}, {lhs, rhs});
            node.getNode()->setImmI64(static_cast<int64_t>(inst.cond));
            setDef(inst.res, node, dag);
        }

        void DAGBuilder::visit(ME::AllocaInst& inst, SelectionDAG& dag)
        {
            size_t dest_id = static_cast<ME::RegOperand*>(inst.res)->getRegNum();

            DataType* ptr_ty = BE::I64;
            SDValue   v      = dag.getFrameIndexNode(static_cast<int>(dest_id), ptr_ty);

            v.getNode()->setIRRegId(dest_id);

            reg_value_map_[dest_id] = v;
        }

        void DAGBuilder::visit(ME::BrCondInst& inst, SelectionDAG& dag)
        {
            // 任务：生成 BRCOND 节点
            // 要求：操作数顺序 [cond(I32), trueLabel(LABEL), falseLabel(LABEL)]
            // 注意：LABEL 节点请通过 getValue(label, dag) 获取
            SDValue cond = getValue(inst.cond, dag, BE::I32);
            SDValue t    = getValue(inst.trueTar, dag);
            SDValue f    = getValue(inst.falseTar, dag);
            dag.getNode(static_cast<unsigned>(ISD::BRCOND), {}, {cond, t, f});
        }

        void DAGBuilder::visit(ME::BrUncondInst& inst, SelectionDAG& dag)
        {
            // 任务：生成无条件分支 BR 节点，操作数为目标 LABEL
            SDValue target = getValue(inst.target, dag);
            dag.getNode(static_cast<unsigned>(ISD::BR), {}, {target});
        }

        void DAGBuilder::visit(ME::GlbVarDeclInst& inst, SelectionDAG& dag)
        {
            (void)inst;
            (void)dag;
            ERROR("GlbVarDeclInst should not appear in DAGBuilder");
        }

        void DAGBuilder::visit(ME::CallInst& inst, SelectionDAG& dag)
        {
            // 任务：生成 CALL 节点并维护 Chain
            // 步骤：
            // 1) ops 以 currentChain_ 开头 （考虑为什么？）
            // 2) 追加 callee 符号：dag.getSymNode(ISD::SYMBOL, {BE::PTR}, {}, inst.funcName)
            // 3) 依次追加所有参数（用 mapType 推导参数类型作为 dtype）
            // 4) 有返回值：结果类型为 vt，setDef，并更新 currentChain_ = node
            //    无返回值：结果类型为 {}，同样更新 currentChain_
            std::vector<SDValue> ops;
            ops.push_back(currentChain_);

            SDValue callee = dag.getSymNode(static_cast<uint32_t>(ISD::SYMBOL), {BE::PTR}, {}, inst.funcName);
            ops.push_back(callee);

            for (auto& [argTy, argOp] : inst.args)
            {
                auto    vt = mapType(argTy);
                SDValue a  = getValue(argOp, dag, vt);
                ops.push_back(a);
            }

            bool hasRet = (inst.res != nullptr) && (inst.retType != ME::DataType::VOID);
            if (hasRet)
            {
                auto    vt   = mapType(inst.retType);
                SDValue node = dag.getNode(static_cast<unsigned>(ISD::CALL), {vt, BE::TOKEN}, ops);
                setDef(inst.res, SDValue(node.getNode(), 0), dag);
                currentChain_ = SDValue(node.getNode(), 1);
            }
            else
            {
                SDValue node = dag.getNode(static_cast<unsigned>(ISD::CALL), {BE::TOKEN}, ops);
                currentChain_ = node;
            }
        }

        void DAGBuilder::visit(ME::FuncDeclInst& inst, SelectionDAG& dag)
        {
            (void)inst;
            (void)dag;
            ERROR("FuncDeclInst should not appear in DAGBuilder");
        }
        void DAGBuilder::visit(ME::FuncDefInst& inst, SelectionDAG& dag)
        {
            (void)inst;
            (void)dag;
            ERROR("FuncDefInst should not appear in DAGBuilder");
        }

        [[maybe_unused]]
        static inline int elemByteSize(ME::DataType t)
        {
            switch (t)
            {
                case ME::DataType::I1:
                case ME::DataType::I8:
                case ME::DataType::I32: return 4;
                case ME::DataType::I64:
                case ME::DataType::PTR: return 8;
                case ME::DataType::F32: return 4;
                case ME::DataType::DOUBLE: return 8;
                default: return 4;
            }
        }

        void DAGBuilder::visit(ME::GEPInst& inst, SelectionDAG& dag)
        {
            // 任务：实现 GEP 到地址计算 DAG 的转换
            // 要求：
            // 1) 取 base 指针（BE::PTR）
            // 2) 将各维度下标组合为元素偏移（必要时做 ZEXT 到 I64），再乘以元素字节大小，最后与基址相加
            // 3) 产出 BE::PTR 类型的地址结果并 setDef
            SDValue base = getValue(inst.basePtr, dag, BE::PTR);

            if (inst.idxs.empty())
            {
                setDef(inst.res, base, dag);
                return;
            }

            DataType* idxTy = mapType(inst.idxType);

            SDValue offsetElems = dag.getConstantI64(0, BE::I64);

            auto computeStride = [&](size_t i) -> int64_t {
                if (inst.dims.empty()) return 1;
                if (i + 1 >= inst.dims.size()) return 1;
                int64_t stride = 1;
                for (size_t j = i + 1; j < inst.dims.size(); ++j) stride *= static_cast<int64_t>(inst.dims[j]);
                return stride;
            };

            for (size_t i = 0; i < inst.idxs.size(); ++i)
            {
                SDValue idx = getValue(inst.idxs[i], dag, idxTy);
                if (idxTy != BE::I64)
                    idx = dag.getNode(static_cast<unsigned>(ISD::ZEXT), {BE::I64}, {idx});

                int64_t stride = computeStride(i);
                SDValue term   = idx;
                if (stride != 1)
                {
                    SDValue strideC = dag.getConstantI64(stride, BE::I64);
                    term           = dag.getNode(static_cast<unsigned>(ISD::MUL), {BE::I64}, {idx, strideC});
                }

                offsetElems = dag.getNode(static_cast<unsigned>(ISD::ADD), {BE::I64}, {offsetElems, term});
            }

            int     eSize = elemByteSize(inst.dt);
            SDValue byteOffset;
            if (eSize == 1)
                byteOffset = offsetElems;
            else
            {
                SDValue sizeC = dag.getConstantI64(static_cast<int64_t>(eSize), BE::I64);
                byteOffset    = dag.getNode(static_cast<unsigned>(ISD::MUL), {BE::I64}, {offsetElems, sizeC});
            }

            SDValue addr = dag.getNode(static_cast<unsigned>(ISD::ADD), {BE::PTR}, {base, byteOffset});
            setDef(inst.res, addr, dag);
        }

        void DAGBuilder::visit(ME::ZextInst& inst, SelectionDAG& dag)
        {
            // 任务：实现零扩展 ZEXT（from -> to）
            // 提示：结果类型用 mapType(inst.to)
            auto    fromTy = mapType(inst.from);
            auto    toTy   = mapType(inst.to);
            SDValue src    = getValue(inst.src, dag, fromTy);
            SDValue node   = dag.getNode(static_cast<unsigned>(ISD::ZEXT), {toTy}, {src});
            setDef(inst.dest, node, dag);
        }

        void DAGBuilder::visit(ME::SI2FPInst& inst, SelectionDAG& dag)
        {
            // 任务：实现 SITOFP（有符号整型到浮点）
            SDValue src  = getValue(inst.src, dag, BE::I32);
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::SITOFP), {BE::F32}, {src});
            setDef(inst.dest, node, dag);
        }

        void DAGBuilder::visit(ME::FP2SIInst& inst, SelectionDAG& dag)
        {
            // 任务：实现 FPTOSI（浮点到有符号整型）
            SDValue src  = getValue(inst.src, dag, BE::F32);
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::FPTOSI), {BE::I32}, {src});
            setDef(inst.dest, node, dag);
        }

        void DAGBuilder::visit(ME::PhiInst& inst, SelectionDAG& dag)
        {
            // 任务：为 PHI 构造操作数列表（成对的 LABEL 与 VALUE），结果类型为 mapType(inst.dt)
            // 提示：ops 形如 [LABEL0, VAL0, LABEL1, VAL1, ...]
            std::vector<SDValue> ops;
            auto                 vt = mapType(inst.dt);

            for (auto& [lbl, val] : inst.incomingVals)
            {
                SDValue l = getValue(lbl, dag);
                SDValue v = getValue(val, dag, vt);
                ops.push_back(l);
                ops.push_back(v);
            }

            SDValue node = dag.getNode(static_cast<unsigned>(ISD::PHI), {vt}, ops);
            setDef(inst.res, node, dag);
        }

    }  // namespace DAG
}  // namespace BE
