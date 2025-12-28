#include <middleend/pass/licm.h>

#include <middleend/module/ir_block.h>
#include <middleend/module/ir_instruction.h>
#include <middleend/module/ir_operand.h>
#include <middleend/pass/analysis/analysis_manager.h>
#include <middleend/pass/analysis/cfg.h>
#include <middleend/pass/analysis/dominfo.h>
#include <middleend/pass/analysis/loopanalysis.h>

#include <deque>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ME
{
    static void insertBeforeTerminator(Block* block, Instruction* inst)
    {
        if (!block || !inst) return;
        if (block->insts.empty())
        {
            block->insts.push_back(inst);
            return;
        }

        Instruction* term = block->insts.back();
        if (!term || !term->isTerminator())
        {
            block->insts.push_back(inst);
            return;
        }

        block->insts.pop_back();
        block->insts.push_back(inst);
        block->insts.push_back(term);
    }

    static bool isReg(const Operand* op) { return op && op->getType() == OperandType::REG; }

    static size_t getRegNumOrDie(const Operand* op) { return static_cast<const RegOperand*>(op)->regNum; }

    static Operand* getDefOperand(Instruction* inst)
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
            case Operator::BITXOR:
            case Operator::BITAND:
            case Operator::SHL:
            case Operator::ASHR:
            case Operator::LSHR: {
                auto* a = static_cast<ArithmeticInst*>(inst);
                return a->res;
            }
            case Operator::ICMP: {
                auto* c = static_cast<IcmpInst*>(inst);
                return c->res;
            }
            case Operator::FCMP: {
                auto* c = static_cast<FcmpInst*>(inst);
                return c->res;
            }
            case Operator::ZEXT: {
                auto* z = static_cast<ZextInst*>(inst);
                return z->dest;
            }
            case Operator::SITOFP: {
                auto* s = static_cast<SI2FPInst*>(inst);
                return s->dest;
            }
            case Operator::FPTOSI: {
                auto* f = static_cast<FP2SIInst*>(inst);
                return f->dest;
            }
            case Operator::GETELEMENTPTR: {
                auto* g = static_cast<GEPInst*>(inst);
                return g->res;
            }
            case Operator::CALL: {
                auto* c = static_cast<CallInst*>(inst);
                return c->res;
            }
            case Operator::PHI: {
                auto* p = static_cast<PhiInst*>(inst);
                return p->res;
            }
            case Operator::LOAD: {
                auto* l = static_cast<LoadInst*>(inst);
                return l->res;
            }
            case Operator::ALLOCA: {
                auto* a = static_cast<AllocaInst*>(inst);
                return a->res;
            }
            default: return nullptr;
        }
    }

    static void collectUseOperands(Instruction* inst, std::vector<Operand*>& uses)
    {
        uses.clear();
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
            case Operator::BITXOR:
            case Operator::BITAND:
            case Operator::SHL:
            case Operator::ASHR:
            case Operator::LSHR: {
                auto* a = static_cast<ArithmeticInst*>(inst);
                uses.push_back(a->lhs);
                uses.push_back(a->rhs);
                break;
            }
            case Operator::ICMP: {
                auto* c = static_cast<IcmpInst*>(inst);
                uses.push_back(c->lhs);
                uses.push_back(c->rhs);
                break;
            }
            case Operator::FCMP: {
                auto* c = static_cast<FcmpInst*>(inst);
                uses.push_back(c->lhs);
                uses.push_back(c->rhs);
                break;
            }
            case Operator::ZEXT: {
                auto* z = static_cast<ZextInst*>(inst);
                uses.push_back(z->src);
                break;
            }
            case Operator::SITOFP: {
                auto* s = static_cast<SI2FPInst*>(inst);
                uses.push_back(s->src);
                break;
            }
            case Operator::FPTOSI: {
                auto* f = static_cast<FP2SIInst*>(inst);
                uses.push_back(f->src);
                break;
            }
            case Operator::GETELEMENTPTR: {
                auto* g = static_cast<GEPInst*>(inst);
                uses.push_back(g->basePtr);
                for (auto* idx : g->idxs) uses.push_back(idx);
                break;
            }
            case Operator::LOAD: {
                auto* l = static_cast<LoadInst*>(inst);
                uses.push_back(l->ptr);
                break;
            }
            case Operator::STORE: {
                auto* s = static_cast<StoreInst*>(inst);
                uses.push_back(s->ptr);
                uses.push_back(s->val);
                break;
            }
            case Operator::CALL: {
                auto* c = static_cast<CallInst*>(inst);
                for (auto& [ty, op] : c->args)
                {
                    (void)ty;
                    uses.push_back(op);
                }
                break;
            }
            case Operator::RET: {
                auto* r = static_cast<RetInst*>(inst);
                if (r->res) uses.push_back(r->res);
                break;
            }
            case Operator::BR_COND: {
                auto* br = static_cast<BrCondInst*>(inst);
                uses.push_back(br->cond);
                break;
            }
            case Operator::PHI: {
                auto* p = static_cast<PhiInst*>(inst);
                for (auto& [lbl, val] : p->incomingVals)
                {
                    (void)lbl;
                    if (val) uses.push_back(val);
                }
                break;
            }
            default: break;
        }
    }

    static bool isSpeculatableInvariantOp(Operator op)
    {
        // 保守策略：避免可能触发 UB/异常的指令（如 div/mod/shift），也不外提访存/调用。
        switch (op)
        {
            case Operator::ADD:
            case Operator::SUB:
            case Operator::MUL:
            case Operator::FADD:
            case Operator::FSUB:
            case Operator::FMUL:
            case Operator::ICMP:
            case Operator::FCMP:
            case Operator::BITXOR:
            case Operator::BITAND:
            case Operator::ZEXT:
            case Operator::SITOFP:
            case Operator::FPTOSI:
            case Operator::GETELEMENTPTR: return true;
            default: return false;
        }
    }

    static bool rewritePredBranchTo(Instruction* term, size_t oldTarget, size_t newTarget)
    {
        if (!term) return false;
        if (term->opcode == Operator::BR_UNCOND)
        {
            auto* br = static_cast<BrUncondInst*>(term);
            if (br->target && br->target->getType() == OperandType::LABEL)
            {
                auto* lab = static_cast<LabelOperand*>(br->target);
                if (lab->lnum == oldTarget)
                {
                    br->target = getLabelOperand(newTarget);
                    return true;
                }
            }
            return false;
        }
        if (term->opcode == Operator::BR_COND)
        {
            auto* br = static_cast<BrCondInst*>(term);
            bool  changed = false;
            if (br->trueTar && br->trueTar->getType() == OperandType::LABEL)
            {
                auto* lab = static_cast<LabelOperand*>(br->trueTar);
                if (lab->lnum == oldTarget)
                {
                    br->trueTar = getLabelOperand(newTarget);
                    changed     = true;
                }
            }
            if (br->falseTar && br->falseTar->getType() == OperandType::LABEL)
            {
                auto* lab = static_cast<LabelOperand*>(br->falseTar);
                if (lab->lnum == oldTarget)
                {
                    br->falseTar = getLabelOperand(newTarget);
                    changed      = true;
                }
            }
            return changed;
        }
        return false;
    }

    static Instruction* findTerminator(Block* block)
    {
        if (!block) return nullptr;
        for (auto* inst : block->insts)
            if (inst && inst->isTerminator()) return inst;
        return nullptr;
    }

    // 构造/返回 header 的 preheader。
    // 允许 outside preds 多个：会在 preheader 中为 header 的 phi 创建“聚合 phi”。
    static Block* ensurePreheader(Function& function, Analysis::CFG* cfg, int header, const std::set<int>& loopBlocks,
        bool& changed)
    {
        if (!cfg) return nullptr;
        if ((size_t)header >= cfg->invG_id.size()) return nullptr;

        std::vector<int> outsidePreds;
        for (size_t predId : cfg->invG_id[(size_t)header])
        {
            if (!cfg->id2block.count(predId)) continue;
            int pred = (int)predId;
            if (!loopBlocks.count(pred)) outsidePreds.push_back(pred);
        }

        if (outsidePreds.empty()) return nullptr;

        // 创建新的 preheader
        Block* preheader = function.createBlock();
        changed          = true;

        // preheader: (phi... / hoisted...) ; br label %header
        preheader->insertBack(new BrUncondInst(getLabelOperand((size_t)header)));

        // 1) 重写所有 outsidePreds 的终结跳转：pred -> preheader
        for (int predId : outsidePreds)
        {
            Block* predBlock = function.getBlock((size_t)predId);
            if (!predBlock) continue;
            Instruction* term = findTerminator(predBlock);
            if (!rewritePredBranchTo(term, (size_t)header, preheader->blockId)) continue;
        }

        // 2) 修正 header 内的 PHI：把来自 outsidePreds 的 incoming 迁移到 preheader 中聚合
        Block* headerBlock = function.getBlock((size_t)header);
        if (!headerBlock) return preheader;

        // 收集 header 前缀 phi（通常集中在块开头，但这里保险扫描全块）
        for (auto* inst : headerBlock->insts)
        {
            if (!inst || inst->opcode != Operator::PHI) continue;

            auto* phi = static_cast<PhiInst*>(inst);
            std::vector<std::pair<Operand*, Operand*>> moved;  // (val, label)
            for (int predId : outsidePreds)
            {
                Operand* predLabel = getLabelOperand((size_t)predId);
                auto     it        = phi->incomingVals.find(predLabel);
                if (it == phi->incomingVals.end()) continue;
                moved.push_back({it->second, it->first});
            }

            if (moved.empty()) continue;

            // 在 preheader 插入一个新的 phi 来聚合这些来自 outsidePreds 的值
            Operand* newReg = getRegOperand(function.getNewRegId());
            auto*    aggPhi = new PhiInst(phi->dt, newReg);
            for (auto& [val, lbl] : moved) aggPhi->addIncoming(val, lbl);

            // 从 header 的 phi 移除这些 incoming
            for (auto& [val, lbl] : moved)
            {
                (void)val;
                phi->incomingVals.erase(lbl);
            }

            // header 的 phi 增加来自 preheader 的 incoming
            phi->addIncoming(newReg, getLabelOperand(preheader->blockId));

            // 把 aggPhi 插到 preheader 的 terminator 之前
            insertBeforeTerminator(preheader, aggPhi);
        }

        // CFG 结构变化，后续分析应失效
        return preheader;
    }

    void ScalarLICMPass::runOnFunction(Function& function) { runScalarLICM(function); }

    void ScalarLICMPass::runScalarLICM(Function& function)
    {
        bool changed = false;

        auto* cfg      = Analysis::AM.get<Analysis::CFG>(function);
        auto* domInfo  = Analysis::AM.get<Analysis::DomInfo>(function);
        auto* loopInfo = Analysis::AM.get<Analysis::LoopAnalysis>(function);

        if (!cfg || !domInfo || !loopInfo) return;

        // 预构建 def/uses 信息
        std::unordered_map<size_t, int> regDefBlock;
        std::unordered_map<size_t, Instruction*> regDefInst;
        std::unordered_map<size_t, std::unordered_set<int>> regUseBlocks;

        std::vector<Operand*> uses;

        for (auto& [bid, block] : function.blocks)
        {
            for (auto* inst : block->insts)
            {
                Operand* def = getDefOperand(inst);
                if (isReg(def))
                {
                    size_t r = getRegNumOrDie(def);
                    regDefBlock[r] = (int)bid;
                    regDefInst[r]  = inst;
                }
            }
        }

        for (auto& [bid, block] : function.blocks)
        {
            for (auto* inst : block->insts)
            {
                collectUseOperands(inst, uses);
                for (auto* op : uses)
                {
                    if (!isReg(op)) continue;
                    size_t r = getRegNumOrDie(op);
                    regUseBlocks[r].insert((int)bid);
                }
            }
        }

        // 对每个循环做外提
        for (const auto& loop : loopInfo->getLoops())
        {
            if (loop.header < 0) continue;

            // 构造 preheader（并修正 header phi）
            Block* preheader = ensurePreheader(function, cfg, loop.header, loop.blocks, changed);
            if (!preheader) continue;

            // 为了减少风险：不处理包含异常复杂 CFG 的情况
            // （例如 header 不存在/没有 terminator）
            if (!function.getBlock((size_t)loop.header)) continue;

            // 不变量集合（寄存器）
            std::unordered_set<size_t> invariantRegs;

            // 先把“定义在循环外”的寄存器标为不变量
            for (auto& [reg, defBid] : regDefBlock)
            {
                if (!loop.blocks.count(defBid)) invariantRegs.insert(reg);
            }

            // 找可外提的指令集合
            std::unordered_set<Instruction*> hoistSet;
            bool                             progress = true;

            while (progress)
            {
                progress = false;

                for (int bid : loop.blocks)
                {
                    Block* block = function.getBlock((size_t)bid);
                    if (!block) continue;

                    for (auto* inst : block->insts)
                    {
                        if (!inst) continue;
                        if (hoistSet.count(inst)) continue;
                        if (inst->isTerminator()) continue;
                        if (!isSpeculatableInvariantOp(inst->opcode)) continue;

                        // PHI 必须留在 header
                        if (inst->opcode == Operator::PHI) continue;

                        Operand* def = getDefOperand(inst);
                        if (!isReg(def)) continue;
                        size_t defReg = getRegNumOrDie(def);

                        // 结果必须只在循环内使用（更保守，避免影响循环外支配关系/phi 结构）
                        auto useIt = regUseBlocks.find(defReg);
                        if (useIt != regUseBlocks.end())
                        {
                            bool usedOutside = false;
                            for (int ub : useIt->second)
                            {
                                if (!loop.blocks.count(ub))
                                {
                                    usedOutside = true;
                                    break;
                                }
                            }
                            if (usedOutside) continue;
                        }

                        // 所有使用的寄存器都必须是不变量
                        collectUseOperands(inst, uses);
                        bool ok = true;
                        for (auto* op : uses)
                        {
                            if (!isReg(op)) continue;
                            size_t r = getRegNumOrDie(op);
                            if (!invariantRegs.count(r))
                            {
                                ok = false;
                                break;
                            }
                        }
                        if (!ok) continue;

                        hoistSet.insert(inst);
                        invariantRegs.insert(defReg);
                        progress = true;
                    }
                }
            }

            if (hoistSet.empty()) continue;

            // 对 hoistSet 做依赖拓扑排序，保证 def 在 use 之前
            std::vector<Instruction*> nodes;
            nodes.reserve(hoistSet.size());
            for (auto* inst : hoistSet) nodes.push_back(inst);

            std::unordered_map<Instruction*, int> indeg;
            std::unordered_map<Instruction*, std::vector<Instruction*>> adj;

            for (auto* inst : nodes)
            {
                indeg[inst] = 0;
            }

            for (auto* inst : nodes)
            {
                collectUseOperands(inst, uses);
                for (auto* op : uses)
                {
                    if (!isReg(op)) continue;
                    size_t r = getRegNumOrDie(op);
                    auto   defIt = regDefInst.find(r);
                    if (defIt == regDefInst.end()) continue;
                    Instruction* dep = defIt->second;
                    if (!hoistSet.count(dep)) continue;
                    adj[dep].push_back(inst);
                    indeg[inst]++;
                }
            }

            std::queue<Instruction*> q;
            for (auto* inst : nodes)
                if (indeg[inst] == 0) q.push(inst);

            std::vector<Instruction*> order;
            order.reserve(nodes.size());
            while (!q.empty())
            {
                Instruction* u = q.front();
                q.pop();
                order.push_back(u);
                for (auto* v : adj[u])
                {
                    indeg[v]--;
                    if (indeg[v] == 0) q.push(v);
                }
            }

            // 若存在环（理论不应发生），则放弃该 loop 的外提
            if (order.size() != nodes.size()) continue;

            // 从原 block 中移除 hoisted 指令
            for (int bid : loop.blocks)
            {
                Block* block = function.getBlock((size_t)bid);
                if (!block) continue;
                for (auto it = block->insts.begin(); it != block->insts.end();)
                {
                    Instruction* inst = *it;
                    if (inst && hoistSet.count(inst))
                    {
                        it = block->insts.erase(it);
                        changed = true;
                        continue;
                    }
                    ++it;
                }
            }

            for (auto* inst : order)
            {
                insertBeforeTerminator(preheader, inst);
            }
        }

        if (changed)
        {
            // 结构变化：CFG/Dom/Loop 等都可能失效
            Analysis::AM.invalidate(function);
        }
    }
}  // namespace ME
