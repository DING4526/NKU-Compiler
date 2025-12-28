#ifndef __INTERFACES_MIDDLEEND_ANALYSIS_LOOPANALYSIS_H__
#define __INTERFACES_MIDDLEEND_ANALYSIS_LOOPANALYSIS_H__

#include <middleend/pass/analysis/analysis_manager.h>
#include <middleend/pass/analysis/cfg.h>
#include <middleend/pass/analysis/dominfo.h>

#include <set>
#include <unordered_map>
#include <vector>

namespace ME::Analysis
{
    class LoopAnalysis
    {
      public:
        static inline const size_t TID = getTID<LoopAnalysis>();

        struct Loop
        {
            int              header  = -1;
            std::set<int>    blocks;
            std::set<int>    latches;
            std::set<int>    exits;
            int              parent  = -1;
            std::vector<int> children;
        };

      private:
        std::vector<Loop>               loops;
        std::unordered_map<int, size_t> header2index;
        std::unordered_map<int, size_t> block2innermost;

      public:
        LoopAnalysis()  = default;
        ~LoopAnalysis() = default;

        void build(const CFG& cfg, const DomInfo& domInfo);

        const std::vector<Loop>& getLoops() const { return loops; }

        const Loop* getLoopByHeader(int header) const
        {
            auto it = header2index.find(header);
            if (it == header2index.end()) return nullptr;
            return &loops[it->second];
        }

        const Loop* getInnermostLoopOfBlock(int blockId) const
        {
            auto it = block2innermost.find(blockId);
            if (it == block2innermost.end()) return nullptr;
            return &loops[it->second];
        }
    };

    template <>
    LoopAnalysis* Manager::get<LoopAnalysis>(Function& func);
}  // namespace ME::Analysis

#endif  // __INTERFACES_MIDDLEEND_ANALYSIS_LOOPANALYSIS_H__
