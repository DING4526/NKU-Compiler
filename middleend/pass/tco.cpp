#include <middleend/pass/tco.h>

#include <middleend/module/ir_block.h>
#include <middleend/module/ir_instruction.h>
#include <middleend/module/ir_operand.h>
#include <middleend/pass/analysis/analysis_manager.h>

#include <map>
#include <vector>

namespace ME
{
    static bool isScalarParamType(DataType t)
    {
        // In this framework, bool parameters are usually lowered as I32.
        // Treat only I32/F32 as scalar parameters eligible for TRE.
        return t == DataType::I32 || t == DataType::F32;
    }

    static bool isSelfTailCallBlock(const Function& function, const Block* block, CallInst*& callOut, RetInst*& retOut)
    {
        callOut = nullptr;
        retOut  = nullptr;

        if (!block) return false;
        if (block->insts.size() < 2) return false;

        auto* last = block->insts.back();
        if (!last || last->opcode != Operator::RET) return false;

        auto* prev = *(std::next(block->insts.rbegin()));
        if (!prev || prev->opcode != Operator::CALL) return false;

        auto* call = static_cast<CallInst*>(prev);
        auto* ret  = static_cast<RetInst*>(last);

        if (!function.funcDef) return false;
        if (call->funcName != function.funcDef->funcName) return false;

        // Must directly return the call result (or void).
        if (function.funcDef->retType == DataType::VOID)
        {
            if (call->retType != DataType::VOID) return false;
            if (call->res != nullptr) return false;
            if (ret->res != nullptr) return false;
        }
        else
        {
            if (call->retType != function.funcDef->retType) return false;
            if (ret->rt != function.funcDef->retType) return false;
            if (call->res == nullptr) return false;
            if (ret->res != call->res) return false;
        }

        callOut = call;
        retOut  = ret;
        return true;
    }

    bool TCOPass::runOnFunctionImpl(Function& function)
    {
        if (!function.funcDef) return false;
        if (function.blocks.empty()) return false;

        // Only handle functions whose parameters are scalar (no PTR params).
        const auto& argRegs = function.funcDef->argRegs;
        for (const auto& [argType, argOp] : argRegs)
        {
            (void)argOp;
            if (!isScalarParamType(argType)) return false;
        }

        auto itEntry = function.blocks.find(0);
        if (itEntry == function.blocks.end()) return false;
        Block* entry = itEntry->second;
        if (!entry) return false;

        // Map each incoming argument register to its entry alloca pointer (scalar params are lowered as allocas).
        // Pattern expected in entry block:
        //   %ptr = alloca <type>
        //   store <type> %arg, <type>* %ptr
        std::map<size_t, Operand*> argRegToParamSlot;
        std::map<size_t, DataType> argRegToType;
        std::map<size_t, AllocaInst*> regToAlloca;

        for (auto* inst : entry->insts)
        {
            if (!inst) continue;
            if (inst->opcode != Operator::ALLOCA) continue;
            auto* allocaInst = static_cast<AllocaInst*>(inst);
            if (!allocaInst->dims.empty()) continue;
            if (!allocaInst->res || allocaInst->res->getType() != OperandType::REG) continue;
            regToAlloca[allocaInst->res->getRegNum()] = allocaInst;
        }

        for (auto* inst : entry->insts)
        {
            if (!inst) continue;
            if (inst->opcode != Operator::STORE) continue;
            auto* store = static_cast<StoreInst*>(inst);
            if (!store->ptr || store->ptr->getType() != OperandType::REG) continue;
            if (!store->val || store->val->getType() != OperandType::REG) continue;

            size_t ptrReg = store->ptr->getRegNum();
            size_t valReg = store->val->getRegNum();

            auto itA = regToAlloca.find(ptrReg);
            if (itA == regToAlloca.end()) continue;

            for (const auto& [argType, argOp] : argRegs)
            {
                if (!argOp || argOp->getType() != OperandType::REG) continue;
                if (argOp->getRegNum() != valReg) continue;

                if (argType != itA->second->dt) return false;
                if (store->dt != argType) return false;

                argRegToParamSlot[valReg] = store->ptr;
                argRegToType[valReg]      = argType;
            }
        }

        for (const auto& [argType, argOp] : argRegs)
        {
            if (!argOp || argOp->getType() != OperandType::REG) return false;
            size_t argReg = argOp->getRegNum();
            if (!argRegToParamSlot.count(argReg)) return false;
            if (argRegToType[argReg] != argType) return false;
        }

        struct TailSite
        {
            Block*           block;
            CallInst::argList args;
        };

        std::vector<TailSite> tailSites;
        tailSites.reserve(function.blocks.size());

        for (auto& [blockId, block] : function.blocks)
        {
            (void)blockId;
            CallInst* call = nullptr;
            RetInst*  ret  = nullptr;

            if (!isSelfTailCallBlock(function, block, call, ret)) continue;
            if (call->args.size() != argRegs.size()) continue;

            bool typeOk = true;
            for (size_t i = 0; i < argRegs.size(); ++i)
            {
                if (call->args[i].first != argRegs[i].first)
                {
                    typeOk = false;
                    break;
                }
            }
            if (!typeOk) continue;

            tailSites.push_back({block, call->args});
        }

        if (tailSites.empty()) return false;

        // Safety: only allow the entry prologue to contain *exactly* the parameter allocas (no other allocas).
        // This avoids changing semantics for functions with additional local allocas that would otherwise need
        // per-iteration re-initialization.
        std::map<size_t, bool> paramAllocaRegs;
        for (const auto& [argType, argOp] : argRegs)
        {
            (void)argType;
            size_t aReg = argOp->getRegNum();
            auto* slot  = argRegToParamSlot[aReg];
            if (!slot || slot->getType() != OperandType::REG) return false;
            paramAllocaRegs[slot->getRegNum()] = true;
        }
        for (auto* inst : entry->insts)
        {
            if (!inst) continue;
            if (inst->opcode != Operator::ALLOCA) continue;
            auto* allocaInst = static_cast<AllocaInst*>(inst);
            if (!allocaInst->res || allocaInst->res->getType() != OperandType::REG) continue;
            if (!paramAllocaRegs.count(allocaInst->res->getRegNum()))
            {
                // Has non-param alloca in entry; skip.
                return false;
            }
        }

        // Split the entry block into:
        //   Block0: prologue-only (no predecessors), ends with br to loopHeader
        //   loopHeader: original body/control flow. Tail-recursive sites jump here.
        Operand* oldEntryLabel = getLabelOperand(0);

        // Determine the split point: keep only param allocas and their initial stores in Block0.
        auto isPrologueInst = [&](Instruction* inst) -> bool {
            if (!inst) return false;
            if (inst->opcode == Operator::ALLOCA)
            {
                auto* a = static_cast<AllocaInst*>(inst);
                if (!a->res || a->res->getType() != OperandType::REG) return false;
                return paramAllocaRegs.count(a->res->getRegNum()) != 0;
            }
            if (inst->opcode == Operator::STORE)
            {
                auto* s = static_cast<StoreInst*>(inst);
                if (!s->ptr || s->ptr->getType() != OperandType::REG) return false;
                if (!s->val || s->val->getType() != OperandType::REG) return false;
                size_t valReg = s->val->getRegNum();
                if (!argRegToParamSlot.count(valReg)) return false;
                Operand* slot = argRegToParamSlot[valReg];
                if (!slot || slot->getType() != OperandType::REG) return false;
                return s->ptr->getRegNum() == slot->getRegNum();
            }
            return false;
        };

        size_t splitPos = 0;
        for (auto it = entry->insts.begin(); it != entry->insts.end(); ++it)
        {
            if (!isPrologueInst(*it)) break;
            ++splitPos;
        }
        if (splitPos == 0 || splitPos >= entry->insts.size())
        {
            // No clear prologue to keep, or nothing left for the loop body.
            return false;
        }

        Block* loopHeader = function.createBlock();
        const size_t loopHeaderId = loopHeader->blockId;
        Operand* newHeaderLabel   = getLabelOperand(loopHeaderId);

        // Move [splitPos, end) from entry to loopHeader.
        auto moveIt = std::next(entry->insts.begin(), static_cast<long>(splitPos));
        while (moveIt != entry->insts.end())
        {
            Instruction* inst = *moveIt;
            loopHeader->insts.push_back(inst);
            moveIt = entry->insts.erase(moveIt);
        }

        // Entry must now end with a single unconditional branch.
        entry->insertBack(new BrUncondInst(newHeaderLabel));

        // Update PHI incoming labels: any %Block0 predecessor is now %Block<loopHeaderId>.
        for (auto& [id, blk] : function.blocks)
        {
            (void)id;
            for (auto* inst : blk->insts)
            {
                if (!inst || inst->opcode != Operator::PHI) continue;
                auto* phi = static_cast<PhiInst*>(inst);
                auto  it  = phi->incomingVals.find(oldEntryLabel);
                if (it == phi->incomingVals.end()) continue;
                Operand* val = it->second;
                phi->incomingVals.erase(it);
                phi->addIncoming(val, newHeaderLabel);
            }
        }

        // Rewrite tail-recursive sites: store args, then jump to loopHeader.
        bool changed = false;
        for (const auto& site : tailSites)
        {
            Block* b = site.block;
            if (!b || b->insts.size() < 2) continue;

            auto* last = b->insts.back();
            auto* prev = *(std::next(b->insts.rbegin()));
            if (!last || !prev) continue;
            if (last->opcode != Operator::RET || prev->opcode != Operator::CALL) continue;

            // Remove call+ret.
            auto* tailRet = static_cast<RetInst*>(b->insts.back());
            b->insts.pop_back();
            delete tailRet;

            auto* tailCall = static_cast<CallInst*>(b->insts.back());
            b->insts.pop_back();
            delete tailCall;

            // Write new parameter values.
            for (size_t i = 0; i < argRegs.size(); ++i)
            {
                const auto& [paramType, paramRegOp] = argRegs[i];
                const auto& [argType, argVal]       = site.args[i];
                if (paramType != argType) continue;  // should not happen due to earlier check

                size_t incomingReg = paramRegOp->getRegNum();
                Operand* slotPtr   = argRegToParamSlot[incomingReg];
                b->insertBack(new StoreInst(argType, argVal, slotPtr));
            }

            b->insertBack(new BrUncondInst(newHeaderLabel));
            changed = true;
        }

        if (changed) Analysis::AM.invalidate(function);
        return changed;
    }

    void TCOPass::runOnFunction(Function& function) { runOnFunctionImpl(function); }

}  // namespace ME
