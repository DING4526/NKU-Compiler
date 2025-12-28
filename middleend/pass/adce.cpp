#include "middleend/module/ir_function.h"

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <middleend/module/ir_operand.h>
#include <middleend/pass/analysis/analysis_manager.h>
#include <middleend/pass/analysis/loopanalysis.h>
#include <middleend/pass/adce.h>

namespace ME
{
    namespace
    {
        static bool isInherentlyLive(const Instruction* inst)
        {
            if (!inst) return false;
            switch (inst->opcode)
            {
                case Operator::STORE:
                case Operator::CALL:
                case Operator::RET:
                    return true;
                default:
                    return false;
            }
        }

        static Operand* getDefOperand(Instruction* inst)
        {
            if (!inst) return nullptr;
            switch (inst->opcode)
            {
                case Operator::LOAD:
                    return static_cast<LoadInst*>(inst)->res;
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
                case Operator::GETELEMENTPTR:
                    return static_cast<GEPInst*>(inst)->res;
                case Operator::SITOFP:
                    return static_cast<SI2FPInst*>(inst)->dest;
                case Operator::FPTOSI:
                    return static_cast<FP2SIInst*>(inst)->dest;
                case Operator::ZEXT:
                    return static_cast<ZextInst*>(inst)->dest;
                case Operator::PHI:
                    return static_cast<PhiInst*>(inst)->res;
                case Operator::ALLOCA:
                    return static_cast<AllocaInst*>(inst)->res;
                case Operator::CALL:
                    return static_cast<CallInst*>(inst)->res;
                default:
                    return nullptr;
            }
        }

        static void collectUseOperands(Instruction* inst, std::vector<Operand*>& uses)
        {
            if (!inst) return;
            auto add = [&](Operand* op) {
                if (op && op->getType() == OperandType::REG) uses.push_back(op);
            };

            switch (inst->opcode)
            {
                case Operator::LOAD:
                    add(static_cast<LoadInst*>(inst)->ptr);
                    break;
                case Operator::STORE: {
                    auto* i = static_cast<StoreInst*>(inst);
                    add(i->ptr);
                    add(i->val);
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
                    add(i->lhs);
                    add(i->rhs);
                    break;
                }
                case Operator::ICMP: {
                    auto* i = static_cast<IcmpInst*>(inst);
                    add(i->lhs);
                    add(i->rhs);
                    break;
                }
                case Operator::FCMP: {
                    auto* i = static_cast<FcmpInst*>(inst);
                    add(i->lhs);
                    add(i->rhs);
                    break;
                }
                case Operator::BR_COND:
                    add(static_cast<BrCondInst*>(inst)->cond);
                    break;
                case Operator::CALL: {
                    auto* i = static_cast<CallInst*>(inst);
                    for (auto& arg : i->args) add(arg.second);
                    break;
                }
                case Operator::RET:
                    add(static_cast<RetInst*>(inst)->res);
                    break;
                case Operator::GETELEMENTPTR: {
                    auto* i = static_cast<GEPInst*>(inst);
                    add(i->basePtr);
                    for (auto* idx : i->idxs) add(idx);
                    break;
                }
                case Operator::SITOFP:
                    add(static_cast<SI2FPInst*>(inst)->src);
                    break;
                case Operator::FPTOSI:
                    add(static_cast<FP2SIInst*>(inst)->src);
                    break;
                case Operator::ZEXT:
                    add(static_cast<ZextInst*>(inst)->src);
                    break;
                case Operator::PHI: {
                    auto* i = static_cast<PhiInst*>(inst);
                    for (auto& pair : i->incomingVals) add(pair.second);
                    break;
                }
                case Operator::FUNCDEF: {
                    auto* i = static_cast<FuncDefInst*>(inst);
                    for (auto& arg : i->argRegs) add(arg.second);
                    break;
                }
                default:
                    break;
            }
        }

        // Note: for ADCE we avoid simplifying general conditional branches via reachability
        // in cyclic CFGs (it breaks semantics inside live loops). We only fold dead-loop exits
        // using SCC information.
    }  // namespace

    void ADCEPass::runOnFunction(Function& function)
    {
        bool changed = true;
        while (changed)
        {
            changed = false;

            // IR may have changed in previous iteration; refresh cached analyses (CFG/Dom/Loop).
            ME::Analysis::AM.invalidate(function);

            // Map blocks to dense indices (stable within this iteration)
            std::vector<size_t> blockIds;
            blockIds.reserve(function.blocks.size());
            std::unordered_map<size_t, size_t> id2idx;
            for (auto& [bid, _] : function.blocks)
            {
                id2idx[bid] = blockIds.size();
                blockIds.push_back(bid);
            }

            // Build def map: reg -> defining instruction
            std::unordered_map<size_t, Instruction*> regDef;
            regDef.reserve(function.getMaxReg() + 8);
            std::unordered_map<size_t, size_t> regDefBlock;
            regDefBlock.reserve(function.getMaxReg() + 8);
            for (auto& [bid, block] : function.blocks)
            {
                for (auto* inst : block->insts)
                {
                    Operand* def = getDefOperand(inst);
                    if (def && def->getType() == OperandType::REG)
                    {
                        regDef[def->getRegNum()] = inst;
                        regDefBlock[def->getRegNum()] = bid;
                    }
                }
            }

            // Liveness: mark side-effecting instructions and backward slice via reg defs
            std::unordered_set<Instruction*> live;
            live.reserve(256);
            std::deque<Instruction*> wl;

            auto markLive = [&](Instruction* inst) {
                if (!inst) return;
                if (live.insert(inst).second) wl.push_back(inst);
            };

            for (auto& [_, block] : function.blocks)
            {
                for (auto* inst : block->insts)
                {
                    if (isInherentlyLive(inst)) markLive(inst);
                }
            }

            while (!wl.empty())
            {
                Instruction* cur = wl.front();
                wl.pop_front();

                std::vector<Operand*> uses;
                uses.reserve(4);
                collectUseOperands(cur, uses);
                for (auto* op : uses)
                {
                    if (!op || op->getType() != OperandType::REG) continue;
                    auto it = regDef.find(op->getRegNum());
                    if (it != regDef.end()) markLive(it->second);
                }
            }

            // Fold dead-loop exits using LoopAnalysis.
            // We do this *before* marking BR_COND as live, so that a pure control loop with no
            // side effects and no externally-used values can be skipped (avoid TLE on dead loops).
            auto* loopInfo = ME::Analysis::AM.get<ME::Analysis::LoopAnalysis>(function);
            if (loopInfo)
            {
                const auto& loops = loopInfo->getLoops();

                // Precompute: block -> loops it belongs to (innermost + all parents are included).
                std::unordered_map<int, std::vector<size_t>> block2loops;
                block2loops.reserve(function.blocks.size() * 2);
                for (size_t i = 0; i < loops.size(); ++i)
                {
                    for (int b : loops[i].blocks) block2loops[b].push_back(i);
                }

                std::vector<bool> loopHasLive(loops.size(), false);
                for (size_t i = 0; i < loops.size(); ++i)
                {
                    for (int b : loops[i].blocks)
                    {
                        Block* blk = function.getBlock((size_t)b);
                        if (!blk) continue;
                        for (auto* inst : blk->insts)
                        {
                            if (live.count(inst))
                            {
                                loopHasLive[i] = true;
                                break;
                            }
                        }
                        if (loopHasLive[i]) break;
                    }
                }

                std::vector<bool> loopHasExternalRegUse(loops.size(), false);
                for (auto& [bid, block] : function.blocks)
                {
                    for (auto* inst : block->insts)
                    {
                        std::vector<Operand*> uses;
                        uses.reserve(4);
                        collectUseOperands(inst, uses);
                        for (auto* op : uses)
                        {
                            if (!op || op->getType() != OperandType::REG) continue;
                            auto itDefBlk = regDefBlock.find(op->getRegNum());
                            if (itDefBlk == regDefBlock.end()) continue;

                            int defBlk = (int)itDefBlk->second;
                            int useBlk = (int)bid;
                            auto itLoops = block2loops.find(defBlk);
                            if (itLoops == block2loops.end()) continue;

                            for (size_t loopIdx : itLoops->second)
                            {
                                if (!loops[loopIdx].blocks.count(useBlk))
                                {
                                    loopHasExternalRegUse[loopIdx] = true;
                                }
                            }
                        }
                    }
                }

                for (size_t i = 0; i < loops.size(); ++i)
                {
                    if (loopHasLive[i]) continue;
                    if (loopHasExternalRegUse[i]) continue;
                    if (loops[i].exits.size() != 1) continue;

                    int exitBlk = *loops[i].exits.begin();

                    for (int b : loops[i].blocks)
                    {
                        Block* blk = function.getBlock((size_t)b);
                        if (!blk || blk->insts.empty()) continue;
                        Instruction* term = blk->insts.back();
                        if (!term || term->opcode != Operator::BR_COND) continue;

                        auto* br = static_cast<BrCondInst*>(term);
                        if (!br->trueTar || !br->falseTar) continue;
                        if (br->trueTar->getType() != OperandType::LABEL || br->falseTar->getType() != OperandType::LABEL)
                            continue;

                        int t = (int)static_cast<LabelOperand*>(br->trueTar)->lnum;
                        int f = (int)static_cast<LabelOperand*>(br->falseTar)->lnum;

                        bool tIn = loops[i].blocks.count(t) != 0;
                        bool fIn = loops[i].blocks.count(f) != 0;

                        // Must decide between staying in the loop and exiting via the unique exit.
                        if (tIn && !fIn && f == exitBlk)
                        {
                            blk->insts.pop_back();
                            blk->insts.push_back(new BrUncondInst(br->falseTar, br->comment));
                            changed = true;
                        }
                        else if (fIn && !tIn && t == exitBlk)
                        {
                            blk->insts.pop_back();
                            blk->insts.push_back(new BrUncondInst(br->trueTar, br->comment));
                            changed = true;
                        }
                    }
                }
            }

            // Control flow is always semantically relevant: any remaining conditional branch
            // must keep its condition (and the defs feeding it) alive.
            for (auto& [_, block] : function.blocks)
            {
                if (block->insts.empty()) continue;
                Instruction* term = block->insts.back();
                if (term && term->opcode == Operator::BR_COND) markLive(term);
            }

            while (!wl.empty())
            {
                Instruction* cur = wl.front();
                wl.pop_front();

                std::vector<Operand*> uses;
                uses.reserve(4);
                collectUseOperands(cur, uses);
                for (auto* op : uses)
                {
                    if (!op || op->getType() != OperandType::REG) continue;
                    auto it = regDef.find(op->getRegNum());
                    if (it != regDef.end()) markLive(it->second);
                }
            }

            // Remove dead (non-terminator) instructions
            for (auto& [_, block] : function.blocks)
            {
                auto it = block->insts.begin();
                while (it != block->insts.end())
                {
                    Instruction* inst = *it;
                    if (!inst)
                    {
                        it = block->insts.erase(it);
                        changed = true;
                        continue;
                    }

                    if (inst->isTerminator() || inst->opcode == Operator::GLOBAL_VAR || inst->opcode == Operator::FUNCDECL ||
                        inst->opcode == Operator::FUNCDEF)
                    {
                        ++it;
                        continue;
                    }

                    if (live.count(inst) == 0)
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

            // Prune unreachable blocks (starting from entry block 0 if present, else smallest block id)
            if (!function.blocks.empty())
            {
                size_t entryId = 0;
                if (!function.blocks.count(entryId)) entryId = function.blocks.begin()->first;

                std::unordered_set<size_t> reachable;
                std::deque<size_t>         q;
                reachable.insert(entryId);
                q.push_back(entryId);

                while (!q.empty())
                {
                    size_t bid = q.front();
                    q.pop_front();
                    Block* b = function.getBlock(bid);
                    if (!b || b->insts.empty()) continue;
                    Instruction* term = b->insts.back();
                    if (!term || !term->isTerminator()) continue;

                    auto pushLabel = [&](Operand* op) {
                        if (!op || op->getType() != OperandType::LABEL) return;
                        size_t nid = static_cast<LabelOperand*>(op)->lnum;
                        if (function.blocks.count(nid) && reachable.insert(nid).second) q.push_back(nid);
                    };

                    if (term->opcode == Operator::BR_COND)
                    {
                        auto* br = static_cast<BrCondInst*>(term);
                        pushLabel(br->trueTar);
                        pushLabel(br->falseTar);
                    }
                    else if (term->opcode == Operator::BR_UNCOND)
                    {
                        auto* br = static_cast<BrUncondInst*>(term);
                        pushLabel(br->target);
                    }
                }

                // Remove unreachable blocks from function
                for (auto it = function.blocks.begin(); it != function.blocks.end();)
                {
                    if (reachable.count(it->first) == 0)
                    {
                        delete it->second;
                        it = function.blocks.erase(it);
                        changed = true;
                    }
                    else
                    {
                        ++it;
                    }
                }

                // Clean PHI incomings referencing removed blocks
                for (auto& [_, block] : function.blocks)
                {
                    for (auto itInst = block->insts.begin(); itInst != block->insts.end();)
                    {
                        Instruction* inst = *itInst;
                        if (!inst || inst->opcode != Operator::PHI)
                        {
                            ++itInst;
                            continue;
                        }

                        auto* phi = static_cast<PhiInst*>(inst);
                        for (auto itIn = phi->incomingVals.begin(); itIn != phi->incomingVals.end();)
                        {
                            Operand* lab = itIn->first;
                            if (!lab || lab->getType() != OperandType::LABEL)
                            {
                                itIn = phi->incomingVals.erase(itIn);
                                changed = true;
                                continue;
                            }
                            size_t from = static_cast<LabelOperand*>(lab)->lnum;
                            if (reachable.count(from) == 0)
                            {
                                itIn = phi->incomingVals.erase(itIn);
                                changed = true;
                            }
                            else
                            {
                                ++itIn;
                            }
                        }

                        if (phi->incomingVals.empty())
                        {
                            itInst = block->insts.erase(itInst);
                            changed = true;
                        }
                        else
                        {
                            ++itInst;
                        }
                    }
                }
            }
        }
    }
}  // namespace ME