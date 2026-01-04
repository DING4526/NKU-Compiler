#include <backend/targets/riscv64/passes/lowering/phi_elimination.h>
#include <debug.h>
#include <algorithm>
#include <memory>
#include <set>
#include <utility>

namespace BE::RV64::Passes::Lowering
{
    using namespace BE;
    using namespace BE::RV64;

    void PhiEliminationPass::runOnModule(BE::Module& module, const BE::Targeting::TargetInstrAdapter* adapter)
    {
        if (module.functions.empty()) return;
        for (auto* func : module.functions) runOnFunction(func, adapter);
    }

    void PhiEliminationPass::runOnFunction(BE::Function* func, const BE::Targeting::TargetInstrAdapter* adapter)
    {
        if (!func || !adapter) return;
        if (func->blocks.empty()) return;

        BE::MIR::CFGBuilder builder(adapter);
        std::unique_ptr<BE::MIR::CFG> cfg(builder.buildCFGForFunction(func));
        if (!cfg) return;

        auto collectPhiPrefix = [](BE::Block* block) -> std::vector<BE::PhiInst*> {
            std::vector<BE::PhiInst*> phis;
            if (!block) return phis;
            for (auto* inst : block->insts)
            {
                auto* phi = dynamic_cast<BE::PhiInst*>(inst);
                if (!phi) break;
                phis.push_back(phi);
            }
            return phis;
        };

        auto findInsertPos = [&](BE::Block* block) -> size_t {
            if (!block) return 0;
            size_t pos = block->insts.size();
            while (pos > 0)
            {
                auto* inst = block->insts[pos - 1];
                if (adapter->isReturn(inst) || adapter->isUncondBranch(inst) || adapter->isCondBranch(inst))
                {
                    --pos;
                    continue;
                }
                break;
            }
            return pos;
        };

        auto redirectEdgeBranch = [&](uint32_t fromLabel, uint32_t oldTo, uint32_t newTo) {
            if (!func->blocks.count(fromLabel)) return;
            auto* fromBlock = func->blocks[fromLabel];
            for (auto* inst : fromBlock->insts)
            {
                if (!(adapter->isUncondBranch(inst) || adapter->isCondBranch(inst))) continue;
                int tgt = adapter->extractBranchTarget(inst);
                if (tgt != static_cast<int>(oldTo)) continue;

                auto* ri = dynamic_cast<BE::RV64::Instr*>(inst);
                if (!ri) continue;
                ri->label     = BE::RV64::Label(static_cast<int>(newTo));
                ri->use_label = true;
            }
        };

        auto emitParallelCopies = [&](const std::vector<std::pair<BE::Register, BE::Operand*>>& copies)
            -> std::vector<BE::MInstruction*> {
            struct Copy
            {
                BE::Register dst;
                BE::Operand* src;
            };

            std::vector<Copy> pending;
            pending.reserve(copies.size());
            for (auto& [dst, src] : copies)
            {
                if (!src) continue;
                if (auto* rsrc = dynamic_cast<BE::RegOperand*>(src))
                {
                    if (rsrc->reg == dst) continue;
                }
                pending.push_back(Copy{dst, src});
            }

            std::vector<BE::MInstruction*> out;
            out.reserve(pending.size() + 2);

            while (!pending.empty())
            {
                std::set<BE::Register> dstSet;
                for (auto& c : pending) dstSet.insert(c.dst);

                bool progressed = false;
                for (size_t i = 0; i < pending.size();)
                {
                    auto& c = pending[i];
                    auto* rsrc = dynamic_cast<BE::RegOperand*>(c.src);
                    bool ready = (rsrc == nullptr) || (dstSet.find(rsrc->reg) == dstSet.end());
                    if (!ready)
                    {
                        ++i;
                        continue;
                    }

                    auto* dstOp = new BE::RegOperand(c.dst);
                    if (rsrc)
                    {
                        out.push_back(BE::createMove(dstOp, new BE::RegOperand(rsrc->reg), "phi"));
                    }
                    else if (auto* i32 = dynamic_cast<BE::I32Operand*>(c.src))
                    {
                        out.push_back(BE::createMove(dstOp, i32->val, "phi"));
                    }
                    else if (auto* f32 = dynamic_cast<BE::F32Operand*>(c.src))
                    {
                        out.push_back(BE::createMove(dstOp, f32->val, "phi"));
                    }
                    else
                    {
                        out.push_back(BE::createMove(dstOp, c.src, "phi"));
                    }

                    pending.erase(pending.begin() + static_cast<long>(i));
                    progressed = true;
                }

                if (progressed) continue;

                // Cycle: break with a temporary register.
                size_t pick = 0;
                while (pick < pending.size() && dynamic_cast<BE::RegOperand*>(pending[pick].src) == nullptr) ++pick;
                if (pick >= pending.size())
                {
                    // Shouldn't happen (all immediates would have progressed), but be defensive.
                    auto c = pending.back();
                    pending.pop_back();
                    auto* dstOp = new BE::RegOperand(c.dst);
                    out.push_back(BE::createMove(dstOp, c.src, "phi"));
                    continue;
                }

                auto& c = pending[pick];
                auto* rsrc = dynamic_cast<BE::RegOperand*>(c.src);
                ASSERT(rsrc && "cycle breaking expects register source");

                BE::Register tmp = BE::getVReg(c.dst.dt);
                out.push_back(BE::createMove(new BE::RegOperand(tmp), new BE::RegOperand(rsrc->reg), "phi_tmp"));
                c.src = new BE::RegOperand(tmp);
            }

            return out;
        };

        // 1) Split critical edges for blocks that contain Phi.
        uint32_t nextLabel = cfg->max_label + 1;
        {
            std::vector<uint32_t> blockIds;
            blockIds.reserve(func->blocks.size());
            for (auto& [id, _] : func->blocks) blockIds.push_back(id);

            for (uint32_t toLabel : blockIds)
            {
                if (!func->blocks.count(toLabel)) continue;
                auto* toBlock = func->blocks[toLabel];
                auto  phis    = collectPhiPrefix(toBlock);
                if (phis.empty()) continue;

                if (toLabel >= cfg->inv_graph_id.size()) continue;
                auto preds = cfg->inv_graph_id[toLabel];
                if (preds.size() <= 1) continue;

                for (uint32_t fromLabel : preds)
                {
                    if (fromLabel >= cfg->graph_id.size()) continue;
                    if (cfg->graph_id[fromLabel].size() <= 1) continue;  // not a critical edge

                    // Create a new split block: fromLabel -> split -> toLabel.
                    uint32_t splitLabel = nextLabel++;
                    auto*    splitBlock = new BE::Block(splitLabel);
                    splitBlock->insts.push_back(
                        BE::RV64::createJInst(Operator::JAL, BE::RV64::PR::x0, BE::RV64::Label((int)toLabel)));
                    func->blocks[splitLabel] = splitBlock;

                    redirectEdgeBranch(fromLabel, toLabel, splitLabel);

                    // Update CFG for subsequent splits in this pass.
                    cfg->addNewBlock(splitLabel, splitBlock);
                    cfg->removeEdge(fromLabel, toLabel);
                    cfg->makeEdge(fromLabel, splitLabel);
                    cfg->makeEdge(splitLabel, toLabel);

                    // Update Phi incoming labels: incoming from 'fromLabel' now comes from 'splitLabel'.
                    for (auto* phi : phis)
                    {
                        auto it = phi->incomingVals.find(fromLabel);
                        if (it == phi->incomingVals.end()) continue;
                        auto* op = it->second;
                        phi->incomingVals.erase(it);
                        phi->incomingVals[splitLabel] = op;
                    }
                }
            }
        }

        // 2) Insert copies into predecessors, then remove Phi.
        std::vector<uint32_t> finalBlockIds;
        finalBlockIds.reserve(func->blocks.size());
        for (auto& [id, _] : func->blocks) finalBlockIds.push_back(id);

        for (uint32_t blockId : finalBlockIds)
        {
            if (!func->blocks.count(blockId)) continue;
            auto* block = func->blocks[blockId];
            auto  phis  = collectPhiPrefix(block);
            if (phis.empty()) continue;

            std::map<uint32_t, std::vector<std::pair<BE::Register, BE::Operand*>>> copiesPerPred;
            for (auto* phi : phis)
            {
                for (auto& [predLabel, op] : phi->incomingVals)
                    copiesPerPred[predLabel].push_back({phi->resReg, op});
            }

            for (auto& [predLabel, copies] : copiesPerPred)
            {
                if (!func->blocks.count(predLabel)) continue;
                auto* predBlock = func->blocks[predLabel];
                size_t pos      = findInsertPos(predBlock);

                auto moves = emitParallelCopies(copies);
                for (auto* mi : moves)
                {
                    predBlock->insts.insert(predBlock->insts.begin() + static_cast<long>(pos), mi);
                    ++pos;
                }
            }

            while (!block->insts.empty())
            {
                auto* phi = dynamic_cast<BE::PhiInst*>(block->insts.front());
                if (!phi) break;
                block->insts.pop_front();
                BE::MInstruction::delInst(phi);
            }
        }
    }
}  // namespace BE::RV64::Passes::Lowering
