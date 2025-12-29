#include <middleend/pass/cse.h>
#include <middleend/module/ir_operand.h>

namespace ME
{
    Operand* CSEPass::getInstructionResult(Instruction* inst) const
    {
        switch (inst->opcode)
        {
            case Operator::ADD:
            case Operator::SUB:
            case Operator::MUL:
            case Operator::DIV:
            case Operator::MOD:
            case Operator::FADD:
            case Operator::FSUB:
            case Operator::FMUL:
            case Operator::FDIV:
            case Operator::BITAND:
            case Operator::BITXOR:
            case Operator::SHL:
            case Operator::LSHR:
            case Operator::ASHR:
                return static_cast<ArithmeticInst*>(inst)->res;
            case Operator::ICMP:
                return static_cast<IcmpInst*>(inst)->res;
            case Operator::FCMP:
                return static_cast<FcmpInst*>(inst)->res;
            case Operator::ZEXT:
                return static_cast<ZextInst*>(inst)->dest;
            case Operator::SITOFP:
                return static_cast<SI2FPInst*>(inst)->dest;
            case Operator::FPTOSI:
                return static_cast<FP2SIInst*>(inst)->dest;
            default:
                return nullptr;
        }
    }

    bool CSEPass::operandLessThan(Operand* a, Operand* b) const
    {
        if (!a && !b) return false;
        if (!a) return true;
        if (!b) return false;

        // 按类型优先级排序：常量 < 全局 < 寄存器 < 标签
        auto typeOrder = [](OperandType t) -> int {
            switch (t)
            {
                case OperandType::IMMEI32: return 0;
                case OperandType::IMMEF32: return 1;
                case OperandType::GLOBAL:  return 2;
                case OperandType::REG:     return 3;
                case OperandType::LABEL:   return 4;
                default:                   return 5;
            }
        };

        int orderA = typeOrder(a->getType());
        int orderB = typeOrder(b->getType());

        if (orderA != orderB) return orderA < orderB;

        // 同类型时按值比较
        switch (a->getType())
        {
            case OperandType::IMMEI32:
                return static_cast<ImmeI32Operand*>(a)->value < static_cast<ImmeI32Operand*>(b)->value;
            case OperandType::IMMEF32:
                return static_cast<ImmeF32Operand*>(a)->value < static_cast<ImmeF32Operand*>(b)->value;
            case OperandType::GLOBAL:
                return static_cast<GlobalOperand*>(a)->name < static_cast<GlobalOperand*>(b)->name;
            case OperandType::REG:
                return a->getRegNum() < b->getRegNum();
            case OperandType::LABEL:
                return static_cast<LabelOperand*>(a)->lnum < static_cast<LabelOperand*>(b)->lnum;
            default:
                return false;
        }
    }

    void CSEPass::runOnFunction(Function& function)
    {
        // 对每个基本块做 local CSE
        for (auto& [id, block] : function.blocks)
        {
            // 每个基本块开始时清空映射表
            expr2value.clear();
            regRepl.clear();
            instsToRemove.clear();

            // 从前往后扫描每条指令
            for (auto* inst : block->insts)
            {
                // Step A: 先把这条指令的操作数做"替换归一"
                replaceOperands(inst);

                // Step B: 如果它是"可CSE的纯表达式指令"
                if (isCSEableInst(inst))
                {
                    std::string key = makeExprKey(inst);

                    auto it = expr2value.find(key);
                    if (it != expr2value.end())
                    {
                        // 之前算过同样表达式，当前这条指令是冗余的
                        Operand* res = getInstructionResult(inst);

                        if (res && res->getType() == OperandType::REG)
                        {
                            regRepl[res->getRegNum()] = it->second;
                            instsToRemove.insert(inst);
                        }
                    }
                    else
                    {
                        // 第一次遇到这个表达式，记录下来
                        Operand* res = getInstructionResult(inst);

                        if (res)
                        {
                            expr2value[key] = res;
                        }
                    }
                }
                // Step C: 不是可CSE指令就跳过
            }

            // 最后统一 erase 掉冗余指令
            auto it = block->insts.begin();
            while (it != block->insts.end())
            {
                if (instsToRemove.count(*it))
                {
                    it = block->insts.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    std::string CSEPass::operandKey(Operand* op) const
    {
        if (!op) return "null";

        switch (op->getType())
        {
            case OperandType::REG:
                return "R" + std::to_string(op->getRegNum());
            case OperandType::IMMEI32:
                return "I" + std::to_string(static_cast<ImmeI32Operand*>(op)->value);
            case OperandType::IMMEF32:
                return "F" + op->toString();  // 使用 hex bits 输出，稳定
            case OperandType::GLOBAL:
                return "G" + static_cast<GlobalOperand*>(op)->name;
            case OperandType::LABEL:
                return "L" + std::to_string(static_cast<LabelOperand*>(op)->lnum);
            default:
                return "?";
        }
    }

    bool CSEPass::isCommutative(Operator op) const
    {
        switch (op)
        {
            case Operator::ADD:
            case Operator::MUL:
            case Operator::FADD:
            case Operator::FMUL:
            case Operator::BITAND:
            case Operator::BITXOR:
                return true;
            default:
                return false;
        }
    }

    std::string CSEPass::makeExprKey(Instruction* inst) const
    {
        std::string key;

        switch (inst->opcode)
        {
            case Operator::ADD:
            case Operator::SUB:
            case Operator::MUL:
            case Operator::DIV:
            case Operator::MOD:
            case Operator::FADD:
            case Operator::FSUB:
            case Operator::FMUL:
            case Operator::FDIV:
            case Operator::BITAND:
            case Operator::BITXOR:
            case Operator::SHL:
            case Operator::LSHR:
            case Operator::ASHR: {
                auto* i = static_cast<ArithmeticInst*>(inst);
                Operand* opA = i->lhs;
                Operand* opB = i->rhs;

                // 对可交换运算，把操作数排序后再放入 key，使用确定性的比较
                if (isCommutative(inst->opcode) && operandLessThan(opB, opA))
                {
                    std::swap(opA, opB);
                }

                key = std::to_string(static_cast<int>(inst->opcode)) + "_" +
                      std::to_string(static_cast<int>(i->dt)) + "_" +
                      operandKey(opA) + "_" + operandKey(opB);
                break;
            }
            case Operator::ICMP: {
                auto* i = static_cast<IcmpInst*>(inst);
                key = std::to_string(static_cast<int>(inst->opcode)) + "_" +
                      std::to_string(static_cast<int>(i->dt)) + "_" +
                      std::to_string(static_cast<int>(i->cond)) + "_" +
                      operandKey(i->lhs) + "_" + operandKey(i->rhs);
                break;
            }
            case Operator::FCMP: {
                auto* i = static_cast<FcmpInst*>(inst);
                key = std::to_string(static_cast<int>(inst->opcode)) + "_" +
                      std::to_string(static_cast<int>(i->dt)) + "_" +
                      std::to_string(static_cast<int>(i->cond)) + "_" +
                      operandKey(i->lhs) + "_" + operandKey(i->rhs);
                break;
            }
            case Operator::ZEXT: {
                auto* i = static_cast<ZextInst*>(inst);
                key = std::to_string(static_cast<int>(inst->opcode)) + "_" +
                      std::to_string(static_cast<int>(i->from)) + "_" +
                      std::to_string(static_cast<int>(i->to)) + "_" + operandKey(i->src);
                break;
            }
            case Operator::SITOFP: {
                auto* i = static_cast<SI2FPInst*>(inst);
                key = std::to_string(static_cast<int>(inst->opcode)) + "_" + operandKey(i->src);
                break;
            }
            case Operator::FPTOSI: {
                auto* i = static_cast<FP2SIInst*>(inst);
                key = std::to_string(static_cast<int>(inst->opcode)) + "_" + operandKey(i->src);
                break;
            }
            default:
                key = "";
                break;
        }

        return key;
    }

    bool CSEPass::isCSEableInst(Instruction* inst) const
    {
        switch (inst->opcode)
        {
            // 二元算术指令
            case Operator::ADD:
            case Operator::SUB:
            case Operator::MUL:
            case Operator::DIV:
            case Operator::MOD:
            case Operator::FADD:
            case Operator::FSUB:
            case Operator::FMUL:
            case Operator::FDIV:
            case Operator::BITAND:
            case Operator::BITXOR:
            case Operator::SHL:
            case Operator::LSHR:
            case Operator::ASHR:
            // 比较指令
            case Operator::ICMP:
            case Operator::FCMP:
            // 一元纯指令
            case Operator::ZEXT:
            case Operator::SITOFP:
            case Operator::FPTOSI:
                return true;
            default:
                return false;
        }
    }

    void CSEPass::replaceOperands(Instruction* inst)
    {
        auto replace = [&](Operand*& op) {
            if (op && op->getType() == OperandType::REG)
            {
                auto it = regRepl.find(op->getRegNum());
                if (it != regRepl.end())
                {
                    op = it->second;
                }
            }
        };

        switch (inst->opcode)
        {
            case Operator::LOAD: {
                auto* i = static_cast<LoadInst*>(inst);
                replace(i->ptr);
                break;
            }
            case Operator::STORE: {
                auto* i = static_cast<StoreInst*>(inst);
                replace(i->ptr);
                replace(i->val);
                break;
            }
            case Operator::ADD:
            case Operator::SUB:
            case Operator::MUL:
            case Operator::DIV:
            case Operator::MOD:
            case Operator::FADD:
            case Operator::FSUB:
            case Operator::FMUL:
            case Operator::FDIV:
            case Operator::BITAND:
            case Operator::BITXOR:
            case Operator::SHL:
            case Operator::LSHR:
            case Operator::ASHR: {
                auto* i = static_cast<ArithmeticInst*>(inst);
                replace(i->lhs);
                replace(i->rhs);
                break;
            }
            case Operator::ICMP: {
                auto* i = static_cast<IcmpInst*>(inst);
                replace(i->lhs);
                replace(i->rhs);
                break;
            }
            case Operator::FCMP: {
                auto* i = static_cast<FcmpInst*>(inst);
                replace(i->lhs);
                replace(i->rhs);
                break;
            }
            case Operator::BR_COND: {
                auto* i = static_cast<BrCondInst*>(inst);
                replace(i->cond);
                break;
            }
            case Operator::CALL: {
                auto* i = static_cast<CallInst*>(inst);
                for (auto& arg : i->args) replace(arg.second);
                break;
            }
            case Operator::RET: {
                auto* i = static_cast<RetInst*>(inst);
                replace(i->res);
                break;
            }
            case Operator::GETELEMENTPTR: {
                auto* i = static_cast<GEPInst*>(inst);
                replace(i->basePtr);
                for (auto*& idx : i->idxs) replace(idx);
                break;
            }
            case Operator::SITOFP: {
                auto* i = static_cast<SI2FPInst*>(inst);
                replace(i->src);
                break;
            }
            case Operator::FPTOSI: {
                auto* i = static_cast<FP2SIInst*>(inst);
                replace(i->src);
                break;
            }
            case Operator::ZEXT: {
                auto* i = static_cast<ZextInst*>(inst);
                replace(i->src);
                break;
            }
            case Operator::PHI: {
                auto* i = static_cast<PhiInst*>(inst);
                for (auto& pair : i->incomingVals) replace(pair.second);
                break;
            }
            default:
                break;
        }
    }

}  // namespace ME
