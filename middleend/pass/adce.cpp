#include "middleend/module/ir_function.h"

#include <algorithm>
#include <deque>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <middleend/module/ir_operand.h>
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
            for (auto& [_, block] : function.blocks)
            {
                for (auto* inst : block->insts)
                {
                    Operand* def = getDefOperand(inst);
                    if (def && def->getType() == OperandType::REG)
                    {
                        regDef[def->getRegNum()] = inst;
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

            // CFG successors for reachability computation
            const size_t N = blockIds.size();
            std::vector<std::vector<size_t>> succIdx(N);
            for (auto& [bid, block] : function.blocks)
            {
                auto itIdx = id2idx.find(bid);
                if (itIdx == id2idx.end()) continue;
                size_t bidx = itIdx->second;

                if (block->insts.empty()) continue;
                Instruction* term = block->insts.back();
                if (!term || !term->isTerminator()) continue;

                if (term->opcode == Operator::BR_COND)
                {
                    auto* br = static_cast<BrCondInst*>(term);
                    if (br->trueTar && br->trueTar->getType() == OperandType::LABEL)
                    {
                        size_t tid = static_cast<LabelOperand*>(br->trueTar)->lnum;
                        if (id2idx.count(tid)) succIdx[bidx].push_back(id2idx[tid]);
                    }
                    if (br->falseTar && br->falseTar->getType() == OperandType::LABEL)
                    {
                        size_t fid = static_cast<LabelOperand*>(br->falseTar)->lnum;
                        if (id2idx.count(fid)) succIdx[bidx].push_back(id2idx[fid]);
                    }
                }
                else if (term->opcode == Operator::BR_UNCOND)
                {
                    auto* br = static_cast<BrUncondInst*>(term);
                    if (br->target && br->target->getType() == OperandType::LABEL)
                    {
                        size_t tid = static_cast<LabelOperand*>(br->target)->lnum;
                        if (id2idx.count(tid)) succIdx[bidx].push_back(id2idx[tid]);
                    }
                }
            }

            // Live blocks: blocks containing any live instruction (side effects or needed defs)
            std::vector<bool> blockIsLive(N, false);
            for (auto& [bid, block] : function.blocks)
            {
                size_t bidx = id2idx[bid];
                for (auto* inst : block->insts)
                {
                    if (live.count(inst))
                    {
                        blockIsLive[bidx] = true;
                        break;
                    }
                }
            }

            // Compute SCCs of the CFG (Tarjan) and fold dead-loop exits.
            std::vector<int> index(N, -1), lowlink(N, -1), onstack(N, 0);
            std::vector<size_t> st;
            st.reserve(N);
            int idxCounter = 0;
            int sccCounter = 0;
            std::vector<int> sccId(N, -1);

            std::function<void(size_t)> strongconnect = [&](size_t v) {
                index[v] = lowlink[v] = idxCounter++;
                st.push_back(v);
                onstack[v] = 1;

                for (size_t w : succIdx[v])
                {
                    if (index[w] == -1)
                    {
                        strongconnect(w);
                        lowlink[v] = std::min(lowlink[v], lowlink[w]);
                    }
                    else if (onstack[w])
                    {
                        lowlink[v] = std::min(lowlink[v], index[w]);
                    }
                }

                if (lowlink[v] == index[v])
                {
                    while (true)
                    {
                        size_t w = st.back();
                        st.pop_back();
                        onstack[w] = 0;
                        sccId[w] = sccCounter;
                        if (w == v) break;
                    }
                    sccCounter++;
                }
            };

            for (size_t v = 0; v < N; v++)
            {
                if (index[v] == -1) strongconnect(v);
            }

            // Detect SCCs that define values used outside the SCC.
            // If an SCC produces a value consumed elsewhere, we must not fold it away as a “dead loop”.
            std::unordered_map<size_t, int> regDefScc;
            regDefScc.reserve(function.getMaxReg() + 8);
            for (auto& [bid, block] : function.blocks)
            {
                size_t bidx = id2idx[bid];
                int    sid  = sccId[bidx];
                for (auto* inst : block->insts)
                {
                    Operand* def = getDefOperand(inst);
                    if (def && def->getType() == OperandType::REG)
                    {
                        regDefScc[def->getRegNum()] = sid;
                    }
                }
            }

            std::vector<bool> sccHasExternalRegUse(sccCounter, false);
            for (auto& [bid, block] : function.blocks)
            {
                size_t bidx   = id2idx[bid];
                int    useScc = sccId[bidx];
                for (auto* inst : block->insts)
                {
                    std::vector<Operand*> uses;
                    uses.reserve(4);
                    collectUseOperands(inst, uses);
                    for (auto* op : uses)
                    {
                        if (!op || op->getType() != OperandType::REG) continue;
                        auto it = regDefScc.find(op->getRegNum());
                        if (it != regDefScc.end() && it->second != useScc)
                        {
                            sccHasExternalRegUse[it->second] = true;
                        }
                    }
                }
            }

            std::vector<bool> sccHasLive(sccCounter, false);
            for (size_t v = 0; v < N; v++)
            {
                if (blockIsLive[v]) sccHasLive[sccId[v]] = true;
            }

            // More conservative for control-flow: track SCCs that (directly or indirectly)
            // can reach any inherently-live instruction (CALL/STORE/RET). This helps avoid
            // folding loops that choose between different observable outcomes (e.g., different RETs).
            std::vector<bool> sccHasInherent(sccCounter, false);
            for (auto& [bid, block] : function.blocks)
            {
                size_t bidx = id2idx[bid];
                int    sid  = sccId[bidx];
                for (auto* inst : block->insts)
                {
                    if (isInherentlyLive(inst))
                    {
                        sccHasInherent[sid] = true;
                        break;
                    }
                }
            }

            std::vector<std::unordered_set<int>> sccSuccSet(sccCounter);
            for (size_t v = 0; v < N; v++)
            {
                int sv = sccId[v];
                for (size_t w : succIdx[v])
                {
                    int sw = sccId[w];
                    if (sv != sw) sccSuccSet[sv].insert(sw);
                }
            }

            std::vector<bool> sccCanReachInherent = sccHasInherent;
            bool              reachChanged        = true;
            while (reachChanged)
            {
                reachChanged = false;
                for (int s = 0; s < sccCounter; s++)
                {
                    if (sccCanReachInherent[s]) continue;
                    for (int t : sccSuccSet[s])
                    {
                        if (sccCanReachInherent[t])
                        {
                            sccCanReachInherent[s] = true;
                            reachChanged           = true;
                            break;
                        }
                    }
                }
            }

            auto uniqueLiveExitScc = [&](int scc) -> int {
                int unique = -1;
                for (int t : sccSuccSet[scc])
                {
                    if (!sccCanReachInherent[t]) continue;
                    if (unique == -1)
                    {
                        unique = t;
                    }
                    else if (unique != t)
                    {
                        return -2;  // multiple
                    }
                }
                return unique;  // -1 none, >=0 unique, -2 multiple
            };

            // Only fold BR_COND when it decides whether to stay inside a dead SCC (dead loop)
            // or exit to a different SCC.
            for (auto& [bid, block] : function.blocks)
            {
                if (block->insts.empty()) continue;
                Instruction* term = block->insts.back();
                if (!term || term->opcode != Operator::BR_COND) continue;

                auto* br = static_cast<BrCondInst*>(term);
                if (!br->trueTar || !br->falseTar) continue;
                if (br->trueTar->getType() != OperandType::LABEL || br->falseTar->getType() != OperandType::LABEL)
                    continue;

                size_t tid = static_cast<LabelOperand*>(br->trueTar)->lnum;
                size_t fid = static_cast<LabelOperand*>(br->falseTar)->lnum;
                if (!id2idx.count(bid) || !id2idx.count(tid) || !id2idx.count(fid)) continue;

                size_t bidx = id2idx[bid];
                size_t tidx = id2idx[tid];
                size_t fidx = id2idx[fid];

                int sccB = sccId[bidx];
                int sccT = sccId[tidx];
                int sccF = sccId[fidx];

                // If taking one edge keeps us in the same SCC and the other leaves it,
                // we can skip the SCC ONLY when:
                // - the SCC has no dataflow-live instructions (does not compute values used later)
                // - and it has a single live exit (does not choose between different observable outcomes)
                // This approximates safe removal of dead loops.
                if (!sccHasLive[sccB] && !sccHasExternalRegUse[sccB])
                {
                    if (sccT == sccB && sccF != sccB)
                    {
                        int u = uniqueLiveExitScc(sccB);
                        if (u >= 0 && u == sccF)
                        {
                            block->insts.pop_back();
                            block->insts.push_back(new BrUncondInst(br->falseTar, br->comment));
                            changed = true;
                        }
                    }
                    else if (sccF == sccB && sccT != sccB)
                    {
                        int u = uniqueLiveExitScc(sccB);
                        if (u >= 0 && u == sccT)
                        {
                            block->insts.pop_back();
                            block->insts.push_back(new BrUncondInst(br->trueTar, br->comment));
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