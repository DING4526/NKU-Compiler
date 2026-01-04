#include <backend/ra/linear_scan.h>
#include <backend/mir/m_function.h>
#include <backend/mir/m_instruction.h>
#include <backend/mir/m_block.h>
#include <backend/mir/m_defs.h>
#include <backend/target/target_reg_info.h>
#include <backend/target/target_instr_adapter.h>
#include <backend/common/cfg.h>
#include <backend/common/cfg_builder.h>
#include <utils/dynamic_bitset.h>
#include <debug.h>

#include <map>
#include <set>
#include <deque>
#include <algorithm>

namespace BE::RA
{
    /*
     * 线性扫描寄存器分配（Linear Scan）教学版说明
     *
     * 目标：将每个虚拟寄存器（vreg）的活跃区间映射到目标机的物理寄存器或栈槽（溢出）。
     *
     * 核心步骤（整数/浮点分开执行，流程相同）：
     * 1) 指令线性化与编号：为函数内所有指令分配全局顺序号，记录每个基本块的 [start, end) 区间，
     *    同时收集调用点（callPoints），用于偏好分配被调用者保存寄存器（callee-saved）。
     * 2) 构建 USE/DEF：枚举每条指令的使用与定义寄存器，聚合到基本块级的 USE/DEF 集合。
     * 3) 活跃性分析：在 CFG 上迭代 IN/OUT，满足 IN = USE ∪ (OUT − DEF) 直至收敛。
     * 4) 活跃区间构建：按基本块从后向前，根据 IN/OUT 与指令次序，累积每个 vreg 的若干 [start, end) 段并合并。
     * 5) 标记跨调用：若区间与任意调用点重叠（交叉），标记 crossesCall=true，以便后续优先使用被调用者保存寄存器。
     * 6) 线性扫描分配：将区间按起点排序，维护活动集合 active；到达新区间时先移除已过期区间，然后
     *    尝试选择空闲物理寄存器；若无空闲则选择一个区间溢出（常见启发：溢出“结束点更远”的区间）。
     * 7) 重写 MIR：对未分配物理寄存器的 use/def，在指令前/后插入 reload/spill，并用临时物理寄存器替换操作数。
     *
     * 提示：
     * - 通过 TargetInstrAdapter 提供的接口完成目标无关的指令读写。
     * - TargetRegInfo 提供了可分配寄存器集合、被调用者保存寄存器、保留寄存器等信息。
     */
    namespace
    {
        struct Segment
        {
            int start;
            int end;
            Segment(int s = 0, int e = 0) : start(s), end(e) {}
        };
        struct Interval
        {
            BE::Register         vreg;
            std::vector<Segment> segs;
            bool                 crossesCall = false;

            void addSegment(int s, int e)
            {
                if (s >= e) return;
                segs.emplace_back(s, e);
            }

            int start() const
            {
                if (segs.empty()) return 0;
                int s = segs.front().start;
                for (auto& seg : segs) s = std::min(s, seg.start);
                return s;
            }
            int end() const
            {
                if (segs.empty()) return 0;
                int e = segs.front().end;
                for (auto& seg : segs) e = std::max(e, seg.end);
                return e;
            }

            void merge()
            {
                if (segs.empty()) return;

                std::sort(segs.begin(), segs.end(), [](const Segment& a, const Segment& b) {
                    if (a.start != b.start) return a.start < b.start;
                    return a.end < b.end;
                });

                std::vector<Segment> merged;
                merged.reserve(segs.size());
                Segment cur = segs.front();
                for (size_t i = 1; i < segs.size(); ++i)
                {
                    const auto& s = segs[i];
                    if (s.start <= cur.end)  // overlap / adjacent
                        cur.end = std::max(cur.end, s.end);
                    else
                    {
                        merged.push_back(cur);
                        cur = s;
                    }
                }
                merged.push_back(cur);
                segs.swap(merged);
            }
        };

        struct IntervalOrder
        {
            bool operator()(const Interval* a, const Interval* b) const
            {
                if (a == b) return false;
                if (!a) return true;
                if (!b) return false;
                int as = a->start();
                int bs = b->start();
                if (as != bs) return as < bs;
                int ae = a->end();
                int be = b->end();
                if (ae != be) return ae < be;
                return a->vreg.rId < b->vreg.rId;
            }
        };
    }  // namespace

    static std::vector<int> buildAllocatableInt(const BE::Targeting::TargetRegInfo& ri)
    {
        std::set<int> reserved(ri.reservedRegs().begin(), ri.reservedRegs().end());
        std::vector<int> out;
        for (int r : ri.intRegs())
        {
            if (reserved.count(r)) continue;
            out.push_back(r);
        }
        return out;
    }
    static std::vector<int> buildAllocatableFloat(const BE::Targeting::TargetRegInfo& ri)
    {
        std::set<int> reserved(ri.reservedRegs().begin(), ri.reservedRegs().end());
        std::vector<int> out;
        for (int r : ri.floatRegs())
        {
            if (reserved.count(r)) continue;
            out.push_back(r);
        }
        return out;
    }

    void LinearScanRA::allocateFunction(BE::Function& func, const BE::Targeting::TargetRegInfo& regInfo)
    {
        ASSERT(BE::Targeting::g_adapter && "TargetInstrAdapter is not set");

        std::map<BE::Block*, std::pair<int, int>>                                   blockRange;
        std::vector<std::pair<BE::Block*, std::deque<BE::MInstruction*>::iterator>> id2iter;
        std::set<int>                                                               callPoints;
        int                                                                         ins_id = 0;
        for (auto& [bid, block] : func.blocks)
        {
            int start = ins_id;
            for (auto it = block->insts.begin(); it != block->insts.end(); ++it, ++ins_id)
            {
                id2iter.emplace_back(block, it);
                if (BE::Targeting::g_adapter->isCall(*it)) callPoints.insert(ins_id);
            }
            blockRange[block] = {start, ins_id};
        }

        std::map<BE::Block*, std::set<BE::Register>> USE, DEF;
        for (auto& [bid, block] : func.blocks)
        {
            std::set<BE::Register> use, def;
            for (auto it = block->insts.begin(); it != block->insts.end(); ++it)
            {
                std::vector<BE::Register> uses, defs;
                BE::Targeting::g_adapter->enumUses(*it, uses);
                BE::Targeting::g_adapter->enumDefs(*it, defs);
                for (auto& d : defs)
                {
                    if (!d.isVreg) continue;
                    if (!def.count(d)) def.insert(d);
                }
                for (auto& u : uses)
                {
                    if (!u.isVreg) continue;
                    if (!def.count(u)) use.insert(u);
                }
            }
            USE[block] = std::move(use);
            DEF[block] = std::move(def);
        }

        // ============================================================================
        // 构建 CFG 后继关系
        // ============================================================================
        // 作用：搭建活跃性数据流的图结构。
        // 如何做：可直接用 MIR::CFGBuilder 生成 CFG，再转换为 succs 映射。
        BE::MIR::CFG*                                 cfg = nullptr;
        std::map<BE::Block*, std::vector<BE::Block*>> succs;
        {
            BE::MIR::CFGBuilder builder(BE::Targeting::g_adapter);
            cfg = builder.buildCFGForFunction(&func);

            for (auto& [bid, block] : func.blocks)
            {
                if (!block) continue;
                if (cfg && bid < cfg->graph.size())
                    succs[block] = cfg->graph[bid];
                else
                    succs[block] = {};
            }
        }

        // ============================================================================
        // 活跃性分析（IN/OUT）
        // ============================================================================
        // IN[b] = USE[b] ∪ (OUT[b] − DEF[b])，OUT[b] = ⋃ IN[s]，其中 s ∈ succs[b]
        // 迭代执行上述操作直到不变为止
        std::map<BE::Block*, std::set<BE::Register>> IN, OUT;
        bool                                         changed = true;
        while (changed)
        {
            changed = false;
            for (auto& [bid, block] : func.blocks)
            {
                std::set<BE::Register> newOUT;
                for (auto* s : succs[block])
                {
                    auto it = IN.find(s);
                    if (it != IN.end()) newOUT.insert(it->second.begin(), it->second.end());
                }
                std::set<BE::Register> newIN = USE[block];

                for (auto& r : newOUT)
                    if (!DEF[block].count(r)) newIN.insert(r);

                if (!(newOUT != OUT[block] || newIN != IN[block])) continue;

                OUT[block] = std::move(newOUT);
                IN[block]  = std::move(newIN);
                changed    = true;
            }
        }

        delete cfg;

        // ============================================================================
        // 构建活跃区间（Intervals）
        // ============================================================================
        // 作用：得到每个 vreg 的若干 [start,end) 段并合并（interval.merge()）。
        // 如何做：对每个基本块，反向遍历其指令序列，根据 IN/OUT/uses/defs 更新段的开始/结束。
        std::map<BE::Register, Interval> intervals;
        auto ensureInterval = [&](const BE::Register& r) -> Interval& {
            auto it = intervals.find(r);
            if (it != intervals.end()) return it->second;
            Interval iv;
            iv.vreg = r;
            auto [newIt, _] = intervals.emplace(r, iv);
            return newIt->second;
        };

        for (auto& [bid, block] : func.blocks)
        {
            if (!block) continue;
            auto rangeIt = blockRange.find(block);
            if (rangeIt == blockRange.end()) continue;
            int bStart = rangeIt->second.first;
            int bEnd   = rangeIt->second.second;
            if (bStart >= bEnd) continue;

            std::set<BE::Register> live = OUT[block];
            std::map<BE::Register, size_t> segIndexInBlock;

            for (auto& r : live)
            {
                auto& iv = ensureInterval(r);
                iv.addSegment(bStart, bEnd);
                segIndexInBlock[r] = iv.segs.size() - 1;
            }

            int pos = bEnd - 1;
            for (auto it = block->insts.rbegin(); it != block->insts.rend(); ++it, --pos)
            {
                std::vector<BE::Register> uses, defs;
                BE::Targeting::g_adapter->enumUses(*it, uses);
                BE::Targeting::g_adapter->enumDefs(*it, defs);

                for (auto& u : uses)
                {
                    if (!u.isVreg) continue;
                    if (!live.count(u))
                    {
                        live.insert(u);
                        auto& iv = ensureInterval(u);
                        iv.addSegment(bStart, pos + 1);
                        segIndexInBlock[u] = iv.segs.size() - 1;
                    }
                }

                for (auto& d : defs)
                {
                    if (!d.isVreg) continue;
                    auto& iv = ensureInterval(d);

                    if (live.count(d))
                    {
                        auto idxIt = segIndexInBlock.find(d);
                        if (idxIt != segIndexInBlock.end())
                        {
                            iv.segs[idxIt->second].start = std::min(iv.segs[idxIt->second].start, pos);
                        }
                        live.erase(d);
                        segIndexInBlock.erase(d);
                    }
                    else
                    {
                        // dead def
                        iv.addSegment(pos, pos + 1);
                    }
                }
            }
        }

        for (auto& [r, iv] : intervals) iv.merge();

        for (auto& [r, iv] : intervals)
        {
            for (auto& seg : iv.segs)
            {
                auto it = callPoints.lower_bound(seg.start);
                if (it != callPoints.end() && *it < seg.end)
                {
                    iv.crossesCall = true;
                    break;
                }
            }
        }

        // ============================================================================
        // 线性扫描主循环
        // ============================================================================
        // 作用：按区间起点排序；进入新区间前，先从活动集合 active 移除“已结束”的区间；
        // 然后尝试分配空闲物理寄存器；若无可用，执行溢出策略（如“溢出结束点更远”的区间）。
        auto allIntRegs   = buildAllocatableInt(regInfo);
        auto allFloatRegs = buildAllocatableFloat(regInfo);

        std::map<BE::Register, int> assignedPhys;       // vreg -> physRegId, -1 means spilled
        std::map<BE::Register, int> spillFrameIndex;    // vreg -> FI

        auto ensureSpillSlot = [&](const BE::Register& vreg) -> int {
            auto it = spillFrameIndex.find(vreg);
            if (it != spillFrameIndex.end()) return it->second;
            int sizeBytes = vreg.dt ? vreg.dt->getDataWidth() : 8;
            int fi        = func.frameInfo.createSpillSlot(sizeBytes, 8);
            spillFrameIndex[vreg] = fi;
            return fi;
        };

        auto allocateOneClass = [&](std::vector<Interval*>& work, const std::vector<int>& allRegs,
                                    const std::vector<int>& calleeSaved) {
            std::sort(work.begin(), work.end(), IntervalOrder{});

            std::vector<Interval*> active;

            auto intervalEnd = [](const Interval* iv) { return iv ? iv->end() : 0; };

            auto expireOld = [&](int curStart) {
                std::vector<Interval*> kept;
                kept.reserve(active.size());
                for (auto* iv : active)
                {
                    if (!iv) continue;
                    if (iv->end() <= curStart) continue;
                    kept.push_back(iv);
                }
                active.swap(kept);
                std::sort(active.begin(), active.end(), [&](const Interval* a, const Interval* b) {
                    return intervalEnd(a) < intervalEnd(b);
                });
            };

            auto pickFreeReg = [&](const Interval* iv, const std::set<int>& freeSet) -> int {
                if (!iv) return -1;
                if (freeSet.empty()) return -1;

                auto tryPickFrom = [&](const std::vector<int>& order) -> int {
                    for (int r : order)
                        if (freeSet.count(r)) return r;
                    return -1;
                };

                if (iv->crossesCall)
                {
                    int r = tryPickFrom(calleeSaved);
                    if (r >= 0) return r;
                    // For intervals crossing calls, spill rather than use caller-saved regs to avoid call-site clobbering
                    return -1;
                }
                return tryPickFrom(allRegs);
            };

            for (auto* cur : work)
            {
                if (!cur) continue;
                int curStart = cur->start();
                int curEnd   = cur->end();

                expireOld(curStart);

                std::set<int> freeSet(allRegs.begin(), allRegs.end());
                for (auto* iv : active)
                {
                    auto it = assignedPhys.find(iv->vreg);
                    if (it != assignedPhys.end() && it->second >= 0) freeSet.erase(it->second);
                }

                int reg = pickFreeReg(cur, freeSet);
                if (reg >= 0)
                {
                    assignedPhys[cur->vreg] = reg;
                    active.push_back(cur);
                    std::sort(active.begin(), active.end(), [&](const Interval* a, const Interval* b) {
                        return intervalEnd(a) < intervalEnd(b);
                    });
                    continue;
                }

                // spill
                if (active.empty())
                {
                    ensureSpillSlot(cur->vreg);
                    assignedPhys[cur->vreg] = -1;
                    continue;
                }

                Interval* spill = active.back();
                int       spillEnd = spill->end();
                if (spillEnd > curEnd)
                {
                    int spilledReg = assignedPhys[spill->vreg];
                    ensureSpillSlot(spill->vreg);
                    assignedPhys[spill->vreg] = -1;

                    assignedPhys[cur->vreg] = spilledReg;
                    active.pop_back();
                    active.push_back(cur);
                    std::sort(active.begin(), active.end(), [&](const Interval* a, const Interval* b) {
                        return intervalEnd(a) < intervalEnd(b);
                    });
                }
                else
                {
                    ensureSpillSlot(cur->vreg);
                    assignedPhys[cur->vreg] = -1;
                }
            }
        };

        std::vector<Interval*> intWork;
        std::vector<Interval*> floatWork;
        for (auto& [r, iv] : intervals)
        {
            if (!r.isVreg) continue;
            if (!r.dt) continue;
            if (r.dt->dt == BE::DataType::Type::FLOAT)
                floatWork.push_back(&iv);
            else
                intWork.push_back(&iv);
        }

        allocateOneClass(intWork, allIntRegs, regInfo.calleeSavedIntRegs());
        allocateOneClass(floatWork, allFloatRegs, regInfo.calleeSavedFloatRegs());

        // ============================================================================
        // 重写 MIR（插入 reload/spill，替换 use/def）
        // ============================================================================
        // 作用：将未分配物理寄存器的 use/def 改写为使用 scratch + FILoad/FIStore（由 Adapter 注入）。
        // 如何做：
        // - 对每条指令枚举 uses：若该 vreg 分配了物理寄存器，则直接替换；
        //   否则在指令前插入 reload 到一个 scratch，然后用 scratch 替换 use。
        // - 对每条指令枚举 defs：若分配了物理寄存器则直接替换；
        //   否则先将 def 写到一个 scratch，再在指令后插入 spill 到对应 FI。
        auto pickScratch = [&](const std::vector<int>& allRegs, const BE::Register& like,
                               const std::set<int>& used) -> BE::Register {
            for (int rid : allRegs)
            {
                if (used.count(rid)) continue;
                return BE::Register(rid, like.dt, false);
            }
            ERROR("No scratch physical register available");
            return BE::Register();
        };

        for (auto& [bid, block] : func.blocks)
        {
            if (!block) continue;
            for (size_t idx = 0; idx < block->insts.size(); ++idx)
            {
                auto it = block->insts.begin() + static_cast<std::ptrdiff_t>(idx);
                auto* inst = *it;
                if (!inst) continue;

                std::set<int> usedPhys;
                {
                    std::vector<BE::Register> phys;
                    BE::Targeting::g_adapter->enumPhysRegs(inst, phys);
                    for (auto& r : phys) usedPhys.insert(static_cast<int>(r.rId));
                }

                // uses
                {
                    std::vector<BE::Register> uses;
                    BE::Targeting::g_adapter->enumUses(inst, uses);

                    for (auto& u : uses)
                    {
                        if (!u.isVreg) continue;

                        auto itA = assignedPhys.find(u);
                        int  physId = (itA != assignedPhys.end()) ? itA->second : -1;

                        if (physId >= 0)
                        {
                            BE::Register physReg(physId, u.dt, false);
                            BE::Targeting::g_adapter->replaceUse(inst, u, physReg);
                            usedPhys.insert(physId);
                        }
                        else
                        {
                            int fi = ensureSpillSlot(u);
                            const auto& pool = (u.dt && u.dt->dt == BE::DataType::Type::FLOAT) ? allFloatRegs : allIntRegs;
                            BE::Register scratch = pickScratch(pool, u, usedPhys);

                            // 插在当前指令之前：会导致当前指令 index 后移
                            it = block->insts.begin() + static_cast<std::ptrdiff_t>(idx);
                            BE::Targeting::g_adapter->insertReloadBefore(block, it, scratch, fi);
                            idx++;
                            it = block->insts.begin() + static_cast<std::ptrdiff_t>(idx);
                            inst = *it;

                            BE::Targeting::g_adapter->replaceUse(inst, u, scratch);
                            usedPhys.insert(static_cast<int>(scratch.rId));
                        }
                    }
                }

                // defs
                {
                    std::vector<BE::Register> defs;
                    BE::Targeting::g_adapter->enumDefs(inst, defs);
                    for (auto& d : defs)
                    {
                        if (!d.isVreg) continue;

                        auto itA = assignedPhys.find(d);
                        int  physId = (itA != assignedPhys.end()) ? itA->second : -1;

                        if (physId >= 0)
                        {
                            BE::Register physReg(physId, d.dt, false);
                            BE::Targeting::g_adapter->replaceDef(inst, d, physReg);
                            usedPhys.insert(physId);
                        }
                        else
                        {
                            int fi = ensureSpillSlot(d);
                            const auto& pool = (d.dt && d.dt->dt == BE::DataType::Type::FLOAT) ? allFloatRegs : allIntRegs;
                            BE::Register scratch = pickScratch(pool, d, usedPhys);
                            BE::Targeting::g_adapter->replaceDef(inst, d, scratch);
                            usedPhys.insert(static_cast<int>(scratch.rId));

                            it = block->insts.begin() + static_cast<std::ptrdiff_t>(idx);
                            inst = *it;
                            BE::Targeting::g_adapter->insertSpillAfter(block, it, scratch, fi);
                        }
                    }
                }
            }
        }
    }
}  // namespace BE::RA
