#include <middleend/pass/analysis/loopanalysis.h>

#include <middleend/pass/analysis/analysis_manager.h>
#include <middleend/pass/analysis/cfg.h>
#include <middleend/pass/analysis/dominfo.h>

namespace ME::Analysis
{
    static bool dominates(const std::vector<int>& immDom, int dominator, int node)
    {
        if (dominator < 0 || node < 0) return false;
        if (dominator == node) return true;
        if ((size_t)dominator >= immDom.size() || (size_t)node >= immDom.size()) return false;

        int current = node;
        for (size_t guard = 0; guard < immDom.size(); ++guard)
        {
            int idom = immDom[(size_t)current];
            if (idom == current) break;
            if (idom < 0 || (size_t)idom >= immDom.size()) break;
            current = idom;
            if (current == dominator) return true;
        }
        return false;
    }

    void LoopAnalysis::build(const CFG& cfg, const DomInfo& domInfo)
    {
        loops.clear();
        header2index.clear();
        block2innermost.clear();

        const auto& immDom = domInfo.getImmDom();

        // Step 1: find back edges u -> v (v dominates u). Each back edge forms a natural loop.
        for (const auto& [blockId, block] : cfg.id2block)
        {
            int u = (int)blockId;
            if ((size_t)u >= cfg.G_id.size()) continue;

            for (size_t succId : cfg.G_id[(size_t)u])
            {
                int v = (int)succId;
                if (cfg.id2block.find(succId) == cfg.id2block.end()) continue;
                if (!dominates(immDom, v, u)) continue;

                std::set<int> naturalLoop;
                naturalLoop.insert(v);
                naturalLoop.insert(u);

                std::vector<int> worklist;
                worklist.push_back(u);

                while (!worklist.empty())
                {
                    int x = worklist.back();
                    worklist.pop_back();

                    if ((size_t)x >= cfg.invG_id.size()) continue;

                    for (size_t predId : cfg.invG_id[(size_t)x])
                    {
                        if (cfg.id2block.find(predId) == cfg.id2block.end()) continue;
                        int pred = (int)predId;
                        if (naturalLoop.insert(pred).second) worklist.push_back(pred);
                    }
                }

                auto it = header2index.find(v);
                if (it == header2index.end())
                {
                    Loop loop;
                    loop.header  = v;
                    loop.blocks  = std::move(naturalLoop);
                    loop.latches = {u};
                    loops.push_back(std::move(loop));
                    header2index[v] = loops.size() - 1;
                }
                else
                {
                    Loop& loop = loops[it->second];
                    loop.blocks.insert(naturalLoop.begin(), naturalLoop.end());
                    loop.latches.insert(u);
                }
            }
        }

        // Step 2: compute exits (successors leaving the loop).
        for (auto& loop : loops)
        {
            for (int b : loop.blocks)
            {
                if (b < 0 || (size_t)b >= cfg.G_id.size()) continue;
                for (size_t succId : cfg.G_id[(size_t)b])
                {
                    if (cfg.id2block.find(succId) == cfg.id2block.end()) continue;
                    int succ = (int)succId;
                    if (!loop.blocks.count(succ)) loop.exits.insert(succ);
                }
            }
        }

        // Step 3: build loop nesting (parent/children) using set containment.
        for (size_t i = 0; i < loops.size(); ++i)
        {
            int    bestParent       = -1;
            size_t bestParentBlocks = (size_t)-1;

            for (size_t j = 0; j < loops.size(); ++j)
            {
                if (i == j) continue;

                const auto& inner = loops[i].blocks;
                const auto& outer = loops[j].blocks;
                if (inner.size() >= outer.size()) continue;

                bool isSubset = true;
                for (int b : inner)
                {
                    if (!outer.count(b))
                    {
                        isSubset = false;
                        break;
                    }
                }
                if (!isSubset) continue;

                if (outer.size() < bestParentBlocks)
                {
                    bestParent       = (int)j;
                    bestParentBlocks = outer.size();
                }
            }

            loops[i].parent = bestParent;
        }

        for (size_t i = 0; i < loops.size(); ++i)
        {
            int parent = loops[i].parent;
            if (parent >= 0) loops[(size_t)parent].children.push_back((int)i);
        }

        // Step 4: compute innermost loop for each block.
        for (size_t loopIndex = 0; loopIndex < loops.size(); ++loopIndex)
        {
            const auto& loopBlocks = loops[loopIndex].blocks;
            for (int b : loopBlocks)
            {
                auto it = block2innermost.find(b);
                if (it == block2innermost.end())
                {
                    block2innermost[b] = loopIndex;
                    continue;
                }

                size_t currentLoopIndex = it->second;
                if (loops[loopIndex].blocks.size() < loops[currentLoopIndex].blocks.size())
                    block2innermost[b] = loopIndex;
            }
        }
    }

    template <>
    LoopAnalysis* Manager::get<LoopAnalysis>(Function& func)
    {
        if (auto* cached = getCached<LoopAnalysis>(func)) return cached;

        auto* cfg     = get<CFG>(func);
        auto* domInfo = get<DomInfo>(func);

        auto* loopInfo = new LoopAnalysis();
        loopInfo->build(*cfg, *domInfo);
        cache<LoopAnalysis>(func, loopInfo);
        return loopInfo;
    }
}  // namespace ME::Analysis
