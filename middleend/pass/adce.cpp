#include "middleend/module/ir_function.h"
#include <middleend/pass/adce.h>
#include <middleend/pass/analysis/analysis_manager.h>
#include <middleend/pass/analysis/cfg.h>
#include <middleend/module/ir_operand.h>

namespace ME {
    void ADCEPass::runOnFunction(Function& function) {
        bool changed = true;
        while (changed)
        {
            changed = false;
            std::map<size_t, int> useCounts;
            std::vector<Instruction*> worklist;

            // 1. Count uses
            for (auto& [id, block] : function.blocks)
            {
                for (auto* inst : block->insts)
                {
                    auto count = [&](Operand* op) {
                        if (op && op->getType() == OperandType::REG)
                        {
                            useCounts[op->getRegNum()]++;
                        }
                    };

                    switch (inst->opcode)
                    {
                        case Operator::LOAD: count(static_cast<LoadInst*>(inst)->ptr); break;
                        case Operator::STORE: {
                            auto* i = static_cast<StoreInst*>(inst);
                            count(i->ptr);
                            count(i->val);
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
                            count(i->lhs);
                            count(i->rhs);
                            break;
                        }
                        case Operator::ICMP: {
                            auto* i = static_cast<IcmpInst*>(inst);
                            count(i->lhs);
                            count(i->rhs);
                            break;
                        }
                        case Operator::FCMP: {
                            auto* i = static_cast<FcmpInst*>(inst);
                            count(i->lhs);
                            count(i->rhs);
                            break;
                        }
                        case Operator::BR_COND: count(static_cast<BrCondInst*>(inst)->cond); break;
                        case Operator::CALL: {
                            auto* i = static_cast<CallInst*>(inst);
                            for (auto& arg : i->args) count(arg.second);
                            break;
                        }
                        case Operator::RET: count(static_cast<RetInst*>(inst)->res); break;
                        case Operator::GETELEMENTPTR: {
                            auto* i = static_cast<GEPInst*>(inst);
                            count(i->basePtr);
                            for (auto* idx : i->idxs) count(idx);
                            break;
                        }
                        case Operator::SITOFP: count(static_cast<SI2FPInst*>(inst)->src); break;
                        case Operator::FPTOSI: count(static_cast<FP2SIInst*>(inst)->src); break;
                        case Operator::ZEXT: count(static_cast<ZextInst*>(inst)->src); break;
                        case Operator::PHI: {
                            auto* i = static_cast<PhiInst*>(inst);
                            for (auto& pair : i->incomingVals) count(pair.second);
                            break;
                        }
                        default: break;
                    }
                }
            }

            // 2. Identify dead instructions
            for (auto& [id, block] : function.blocks)
            {
                auto it = block->insts.begin();
                while (it != block->insts.end())
                {
                    Instruction* inst = *it;
                    bool isDead = false;

                    if (inst->isTerminator() || inst->opcode == Operator::STORE || inst->opcode == Operator::CALL || inst->opcode == Operator::GLOBAL_VAR)
                    {
                        isDead = false;
                    }
                    else
                    {
                        Operand* res = nullptr;
                        switch (inst->opcode)
                        {
                            case Operator::LOAD: res = static_cast<LoadInst*>(inst)->res; break;
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
                            case Operator::ASHR: res = static_cast<ArithmeticInst*>(inst)->res; break;
                            case Operator::ICMP: res = static_cast<IcmpInst*>(inst)->res; break;
                            case Operator::FCMP: res = static_cast<FcmpInst*>(inst)->res; break;
                            case Operator::GETELEMENTPTR: res = static_cast<GEPInst*>(inst)->res; break;
                            case Operator::SITOFP: res = static_cast<SI2FPInst*>(inst)->dest; break;
                            case Operator::FPTOSI: res = static_cast<FP2SIInst*>(inst)->dest; break;
                            case Operator::ZEXT: res = static_cast<ZextInst*>(inst)->dest; break;
                            case Operator::PHI: res = static_cast<PhiInst*>(inst)->res; break;
                            case Operator::ALLOCA: res = static_cast<AllocaInst*>(inst)->res; break;
                            default: break;
                        }

                        if (res && res->getType() == OperandType::REG)
                        {
                            if (useCounts[res->getRegNum()] == 0)
                            {
                                isDead = true;
                            }
                        }
                    }

                    if (isDead)
                    {
                        it = block->insts.erase(it);
                        changed = true;
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }
    }
};