#include "middleend/ir_defs.h"
#include <cstddef>
#include <middleend/pass/mem2reg.h>
#include <middleend/pass/analysis/analysis_manager.h>
#include <middleend/pass/analysis/cfg.h>
#include <middleend/module/ir_operand.h>
#include <unordered_map>

namespace ME
{
    void Mem2RegPass::runOnFunction(Function& function)
    {
        domInfo = Analysis::AM.get<Analysis::DomInfo>(function);
        cfg = Analysis::AM.get<Analysis::CFG>(function);
        promoteMemoryToRegister(function);
    }

    void Mem2RegPass::promoteMemoryToRegister(Function& function)
    {
        stacks.clear();
        promotableAllocas.clear();
        regReplacements.clear();
        phiToAlloca.clear();
        instsToRemove.clear();
        promotableRegMap.clear();
        id2block.clear();
        if(function.blocks.empty()) return;

        for(auto& [id, block] : function.blocks) id2block[id] = block;

        // 找出所有可行reg
        std::map<AllocaInst*, std::vector<Instruction*>> allocaUsers;
        std::map<size_t, AllocaInst*> regToAlloca;

        for (auto& [id, block] : function.blocks)
        {
            for (auto* inst : block->insts)
            {
                if (inst->opcode == Operator::ALLOCA)
                {
                    auto* allocaInst = static_cast<AllocaInst*>(inst);
                    if (allocaInst->res->getType() == OperandType::REG)
                    {
                        regToAlloca[allocaInst->res->getRegNum()] = allocaInst;
                    }
                }
            }
        }

        for (auto& [id, block] : function.blocks)
        {
            for (auto* inst : block->insts)
            {
                auto check = [&](Operand* op) {
                    if (op && op->getType() == OperandType::REG) {
                        auto it = regToAlloca.find(op->getRegNum());
                        if (it != regToAlloca.end())
                            allocaUsers[it->second].push_back(inst);
                    }
                };

                switch (inst->opcode)
                {
                    case Operator::LOAD: {
                        auto* i = static_cast<LoadInst*>(inst);
                        check(i->ptr);
                        break;
                    }
                    case Operator::STORE: {
                        auto* i = static_cast<StoreInst*>(inst);
                        check(i->ptr);
                        check(i->val);
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
                        check(i->lhs);
                        check(i->rhs);
                        break;
                    }
                    case Operator::ICMP: {
                        auto* i = static_cast<IcmpInst*>(inst);
                        check(i->lhs);
                        check(i->rhs);
                        break;
                    }
                    case Operator::FCMP: {
                        auto* i = static_cast<FcmpInst*>(inst);
                        check(i->lhs);
                        check(i->rhs);
                        break;
                    }
                    case Operator::BR_COND: {
                        auto* i = static_cast<BrCondInst*>(inst);
                        check(i->cond);
                        break;
                    }
                    case Operator::CALL: {
                        auto* i = static_cast<CallInst*>(inst);
                        for (auto& arg : i->args) check(arg.second);
                        break;
                    }
                    case Operator::RET: {
                        auto* i = static_cast<RetInst*>(inst);
                        check(i->res);
                        break;
                    }
                    case Operator::GETELEMENTPTR: {
                        auto* i = static_cast<GEPInst*>(inst);
                        check(i->basePtr);
                        for (auto* idx : i->idxs) check(idx);
                        break;
                    }
                    case Operator::SITOFP: {
                        auto* i = static_cast<SI2FPInst*>(inst);
                        check(i->src);
                        break;
                    }
                    case Operator::FPTOSI: {
                        auto* i = static_cast<FP2SIInst*>(inst);
                        check(i->src);
                        break;
                    }
                    case Operator::ZEXT: {
                        auto* i = static_cast<ZextInst*>(inst);
                        check(i->src);
                        break;
                    }
                    case Operator::PHI: {
                        auto* i = static_cast<PhiInst*>(inst);
                        for (auto& pair : i->incomingVals) check(pair.second);
                        break;
                    }
                    default: break;
                }
            }
        }

        for (auto& [reg, allocaInst] : regToAlloca)
        {
            if (isPromotable(allocaInst, allocaUsers))
            {
                if(allocaUsers.find(allocaInst) != allocaUsers.end())
                    promotableAllocas.push_back(allocaInst),
                    promotableRegMap[reg] = allocaInst;
                instsToRemove.insert(allocaInst);
            }
        }

        if (promotableAllocas.empty()) return;

        // 通过支配边界插入 phi 函数
        const auto& domFrontier = domInfo->getDomFrontier();
        std::map<Instruction*, Block*> instToBlock;
        for(auto& [id, block] : function.blocks) {
            for(auto* inst : block->insts) instToBlock[inst] = block;
        }
        for (auto* allocaInst : promotableAllocas)
        {
            std::vector<size_t> W;
            std::unordered_map<size_t, int> defBlocks;
            for (auto* user : allocaUsers[allocaInst])
            {
                if (auto* store = dynamic_cast<StoreInst*>(user))
                    W.emplace_back(instToBlock[store]->blockId),
                    defBlocks[instToBlock[store]->blockId] = 1;
            }

            std::unordered_map<size_t, int> F;
            
            // 处理W可能为空的情况
            for(size_t idx = 0; idx < W.size(); ++idx) {
                size_t X = W[idx];
                if (X >= domFrontier.size()) continue;
                for(int Y_id : domFrontier[X]) {
                    if(F.find(Y_id) == F.end()) {
                        F[Y_id] = 1;
                        if(defBlocks.find(Y_id) == defBlocks.end()) {
                            W.push_back(Y_id);
                        }
                        
                        if (id2block.count(Y_id)) {
                            Block* block = id2block[Y_id];
                            Operand* newReg = getRegOperand(function.getNewRegId());
                            auto* phi = new PhiInst(allocaInst->dt, newReg);
                            block->insertFront(phi);
                            phiToAlloca[phi] = allocaInst;
                        }
                    }
                }
            }
        }

        // Rename
        rename(function.blocks[0]);
        // 删除无用代码
        for (auto& [id, block] : function.blocks)
        {   
            for(auto it = block->insts.begin(); it != block->insts.end(); ) {
                if(instsToRemove.find(*it) != instsToRemove.end())
                    it = block->insts.erase(it);
                else ++it;
            }
        }
    }

    bool Mem2RegPass::isPromotable(AllocaInst* allocaInst, const std::map<AllocaInst*, std::vector<Instruction*>>& allocaUsers)
    {
        if (!allocaInst->dims.empty()) return false;

        auto it = allocaUsers.find(allocaInst);
        if (it == allocaUsers.end()) return true;

        for (auto* user : it->second)
            if(user->opcode != Operator::STORE && user->opcode != Operator::LOAD)
                return false;
        return true;
    }

    void Mem2RegPass::rename(Block* block)
    {
        std::map<AllocaInst*, int> pushCount;

        for (auto* inst : block->insts)
        {   
            if (auto* phi = dynamic_cast<PhiInst*>(inst))
            {
                if (phiToAlloca.count(phi))
                {
                    AllocaInst* alloca = phiToAlloca[phi];
                    stacks[alloca].push(phi->res);
                    pushCount[alloca]++;
                }
            } else break;
        }

        for (auto* inst : block->insts)
        {
            if (instsToRemove.count(inst)) continue;

            replaceOperands(inst);

            if (auto* load = dynamic_cast<LoadInst*>(inst))
            {
                if (load->ptr->getType() == OperandType::REG && promotableRegMap.count(load->ptr->getRegNum())) {
                    AllocaInst* alloca = promotableRegMap[load->ptr->getRegNum()];
                    if (!stacks[alloca].empty()) {
                        regReplacements[load->res->getRegNum()] = stacks[alloca].top();
                    } else {
                        Operand* defVal = nullptr;
                        if (alloca->dt == DataType::I32) defVal = getImmeI32Operand(0);
                        else if (alloca->dt == DataType::F32) defVal = getImmeF32Operand(0.0f);
                        else defVal = getImmeI32Operand(0);
                        regReplacements[load->res->getRegNum()] = defVal;
                    }
                    instsToRemove.insert(load);
                }
            }
            else if (auto* store = dynamic_cast<StoreInst*>(inst))
            {
                if (store->ptr->getType() == OperandType::REG && promotableRegMap.count(store->ptr->getRegNum())) {
                    AllocaInst* alloca = promotableRegMap[store->ptr->getRegNum()];
                    stacks[alloca].push(store->val);
                    pushCount[alloca]++;
                    instsToRemove.insert(store);
                }
            }
        }

        if (block->blockId < cfg->G.size()) {
            for (auto* succ : cfg->G[block->blockId]) {
                 for (auto* inst : succ->insts) {
                     if (auto* phi = dynamic_cast<PhiInst*>(inst)) {
                         if (phiToAlloca.count(phi)) {
                             AllocaInst* alloca = phiToAlloca[phi];
                             Operand* val = nullptr;
                             if (!stacks[alloca].empty()) {
                                 val = stacks[alloca].top();
                             } else {
                                 if (alloca->dt == DataType::I32) val = getImmeI32Operand(0);
                                 else if (alloca->dt == DataType::F32) val = getImmeF32Operand(0.0f);
                                 else val = getImmeI32Operand(0);
                             }
                             phi->addIncoming(val, getLabelOperand(block->blockId));
                         }
                     } else break;
                 }
            }
        }

        const auto& domTreeNodes = domInfo->getDomTree();
        if (block->blockId < domTreeNodes.size()) {
            for (int childId : domTreeNodes[block->blockId]) {
                if (id2block.count(childId)) rename(id2block[childId]);
            }
        }


        for (auto& [alloca, count] : pushCount) {
            for (int i = 0; i < count; ++i) stacks[alloca].pop();
        }
    }

    void Mem2RegPass::replaceOperands(Instruction* inst)
    {
        auto replace = [&](Operand*& op) {
            if (op && op->getType() == OperandType::REG) {
                auto it = regReplacements.find(op->getRegNum());
                if (it != regReplacements.end()) {
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
            default: break;
        }
    }
}
