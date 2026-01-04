#include <backend/targets/riscv64/rv64_instr_adapter.h>
#include <backend/targets/riscv64/rv64_defs.h>
#include <algorithm>
#include <debug.h>

namespace BE::Targeting::RV64
{
    using namespace BE::RV64;

    static OpType getOpType(Operator op)
    {
        switch (op)
        {
#define X(name, type, _asm, latency) \
    case Operator::name: return OpType::type;
            RV64_INSTS
#undef X
            default: ERROR("Unknown RV64 operator: %d", (int)op);
        }
        return OpType::R;
    }

    static void pushArgIntRegs(std::vector<BE::Register>& out, int cnt)
    {
        const BE::Register regs[] = {PR::a0, PR::a1, PR::a2, PR::a3, PR::a4, PR::a5, PR::a6, PR::a7};
        int                n      = std::clamp(cnt, 0, 8);
        for (int i = 0; i < n; ++i) out.push_back(regs[i]);
    }

    static void pushArgFloatRegs(std::vector<BE::Register>& out, int cnt)
    {
        const BE::Register regs[] = {PR::fa0, PR::fa1, PR::fa2, PR::fa3, PR::fa4, PR::fa5, PR::fa6, PR::fa7};
        int                n      = std::clamp(cnt, 0, 8);
        for (int i = 0; i < n; ++i) out.push_back(regs[i]);
    }

    bool InstrAdapter::isCall(BE::MInstruction* inst) const
    {
        auto* ri = dynamic_cast<Instr*>(inst);
        if (!ri) return false;
        if (ri->op == Operator::CALL) return true;
        // 允许直接用 JAL ra, label 作为 call
        if (ri->op == Operator::JAL && ri->use_label && ri->rd == PR::ra) return true;
        return false;
    }

    bool InstrAdapter::isReturn(BE::MInstruction* inst) const
    {
        auto* ri = dynamic_cast<Instr*>(inst);
        if (!ri) return false;
        if (ri->op == Operator::RET) return true;
        // 常见 ret 形式：jalr x0, ra, 0
        if (ri->op == Operator::JALR && ri->rd == PR::x0 && ri->rs1 == PR::ra && ri->imme == 0) return true;
        return false;
    }

    bool InstrAdapter::isUncondBranch(BE::MInstruction* inst) const
    {
        auto* ri = dynamic_cast<Instr*>(inst);
        if (!ri) return false;
        // 仅识别可提取 label 的直接跳转，避免把间接跳转误判导致 CFG fallthrough 缺失
        return ri->op == Operator::JAL && ri->use_label;
    }

    bool InstrAdapter::isCondBranch(BE::MInstruction* inst) const
    {
        auto* ri = dynamic_cast<Instr*>(inst);
        if (!ri) return false;
        return getOpType(ri->op) == OpType::B && ri->use_label;
    }

    int InstrAdapter::extractBranchTarget(BE::MInstruction* inst) const
    {
        auto* ri = dynamic_cast<Instr*>(inst);
        if (!ri) return -1;
        if (!(isUncondBranch(inst) || isCondBranch(inst))) return -1;
        if (!ri->use_label) return -1;

        // Label 可能用 lnum 或 jmp_label 表示目标块号，优先使用 jmp_label。
        if (ri->label.jmp_label >= 0) return ri->label.jmp_label;
        return static_cast<int>(ri->label.lnum);
    }

    void InstrAdapter::enumUses(BE::MInstruction* inst, std::vector<BE::Register>& out) const
    {
        if (!inst) return;

        if (auto* mv = dynamic_cast<BE::MoveInst*>(inst))
        {
            if (auto* regOp = dynamic_cast<BE::RegOperand*>(mv->src)) out.push_back(regOp->reg);
            return;
        }
        if (auto* phi = dynamic_cast<BE::PhiInst*>(inst))
        {
            for (auto& [lbl, op] : phi->incomingVals)
            {
                (void)lbl;
                if (!op) continue;
                if (auto* regOp = dynamic_cast<BE::RegOperand*>(op)) out.push_back(regOp->reg);
            }
            return;
        }
        if (auto* ss = dynamic_cast<BE::FIStoreInst*>(inst))
        {
            out.push_back(ss->src);
            return;
        }
        if (dynamic_cast<BE::FILoadInst*>(inst))
        {
            // reload: no register uses
            return;
        }

        auto* ri = dynamic_cast<Instr*>(inst);
        if (!ri) return;

        switch (getOpType(ri->op))
        {
            case OpType::R:
                out.push_back(ri->rs1);
                out.push_back(ri->rs2);
                return;
            case OpType::I:
                if (ri->op == Operator::RET)
                {
                    out.push_back(PR::ra);
                    return;
                }
                out.push_back(ri->rs1);
                return;
            case OpType::S:
                out.push_back(ri->rs1);  // value
                out.push_back(ri->rs2);  // base
                return;
            case OpType::B:
                out.push_back(ri->rs1);
                out.push_back(ri->rs2);
                return;
            case OpType::U:
            case OpType::J:
                return;
            case OpType::R2:
                out.push_back(ri->rs1);
                return;
            case OpType::R4:
                // 当前 Instr 结构仅显式保存 rs1/rs2
                out.push_back(ri->rs1);
                out.push_back(ri->rs2);
                return;
            case OpType::CALL:
                pushArgIntRegs(out, ri->call_ireg_cnt);
                pushArgFloatRegs(out, ri->call_freg_cnt);
                return;
        }
    }

    void InstrAdapter::enumDefs(BE::MInstruction* inst, std::vector<BE::Register>& out) const
    {
        if (!inst) return;

        if (auto* mv = dynamic_cast<BE::MoveInst*>(inst))
        {
            if (auto* regOp = dynamic_cast<BE::RegOperand*>(mv->dest)) out.push_back(regOp->reg);
            return;
        }
        if (auto* phi = dynamic_cast<BE::PhiInst*>(inst))
        {
            out.push_back(phi->resReg);
            return;
        }
        if (auto* ls = dynamic_cast<BE::FILoadInst*>(inst))
        {
            out.push_back(ls->dest);
            return;
        }
        if (dynamic_cast<BE::FIStoreInst*>(inst))
        {
            // spill: no register defs
            return;
        }

        auto* ri = dynamic_cast<Instr*>(inst);
        if (!ri) return;

        switch (getOpType(ri->op))
        {
            case OpType::R:
            case OpType::U:
            case OpType::R2:
            case OpType::R4:
                out.push_back(ri->rd);
                return;
            case OpType::I:
                if (ri->op == Operator::RET) return;
                if (ri->op == Operator::JALR && ri->rd == PR::x0) return;
                out.push_back(ri->rd);
                return;
            case OpType::J:
                if (ri->rd == PR::x0) return;
                out.push_back(ri->rd);
                return;
            case OpType::S:
            case OpType::B:
                return;
            case OpType::CALL:
                // call/jal 会写 ra；返回值寄存器由调用约定定义
                out.push_back(PR::ra);
                out.push_back(PR::a0);
                out.push_back(PR::fa0);
                return;
        }
    }

    static void replaceReg(BE::Register& slot, const BE::Register& from, const BE::Register& to)
    {
        if (slot == from) slot = to;
    }

    void InstrAdapter::replaceUse(BE::MInstruction* inst, const BE::Register& from, const BE::Register& to) const
    {
        if (!inst) return;
        if (auto* mv = dynamic_cast<BE::MoveInst*>(inst))
        {
            if (auto* regOp = dynamic_cast<BE::RegOperand*>(mv->src))
                if (regOp->reg == from) regOp->reg = to;
            return;
        }
        if (auto* phi = dynamic_cast<BE::PhiInst*>(inst))
        {
            for (auto& [lbl, op] : phi->incomingVals)
            {
                (void)lbl;
                if (!op) continue;
                if (auto* regOp = dynamic_cast<BE::RegOperand*>(op))
                    if (regOp->reg == from) regOp->reg = to;
            }
            return;
        }
        if (auto* ss = dynamic_cast<BE::FIStoreInst*>(inst))
        {
            replaceReg(ss->src, from, to);
            return;
        }

        auto* ri = dynamic_cast<Instr*>(inst);
        if (!ri) return;
        replaceReg(ri->rs1, from, to);
        replaceReg(ri->rs2, from, to);
    }

    void InstrAdapter::replaceDef(BE::MInstruction* inst, const BE::Register& from, const BE::Register& to) const
    {
        if (!inst) return;
        if (auto* mv = dynamic_cast<BE::MoveInst*>(inst))
        {
            if (auto* regOp = dynamic_cast<BE::RegOperand*>(mv->dest))
                if (regOp->reg == from) regOp->reg = to;
            return;
        }
        if (auto* phi = dynamic_cast<BE::PhiInst*>(inst))
        {
            replaceReg(phi->resReg, from, to);
            return;
        }
        if (auto* ls = dynamic_cast<BE::FILoadInst*>(inst))
        {
            replaceReg(ls->dest, from, to);
            return;
        }

        auto* ri = dynamic_cast<Instr*>(inst);
        if (!ri) return;
        replaceReg(ri->rd, from, to);
    }

    void InstrAdapter::enumPhysRegs(BE::MInstruction* inst, std::vector<BE::Register>& out) const
    {
        if (!inst) return;

        if (auto* mv = dynamic_cast<BE::MoveInst*>(inst))
        {
            if (auto* src = dynamic_cast<BE::RegOperand*>(mv->src))
                if (!src->reg.isVreg) out.push_back(src->reg);
            if (auto* dst = dynamic_cast<BE::RegOperand*>(mv->dest))
                if (!dst->reg.isVreg) out.push_back(dst->reg);
            return;
        }
        if (auto* phi = dynamic_cast<BE::PhiInst*>(inst))
        {
            if (!phi->resReg.isVreg) out.push_back(phi->resReg);
            for (auto& [lbl, op] : phi->incomingVals)
            {
                (void)lbl;
                if (!op) continue;
                if (auto* regOp = dynamic_cast<BE::RegOperand*>(op))
                    if (!regOp->reg.isVreg) out.push_back(regOp->reg);
            }
            return;
        }
        if (auto* ls = dynamic_cast<BE::FILoadInst*>(inst))
        {
            if (!ls->dest.isVreg) out.push_back(ls->dest);
            return;
        }
        if (auto* ss = dynamic_cast<BE::FIStoreInst*>(inst))
        {
            if (!ss->src.isVreg) out.push_back(ss->src);
            return;
        }

        auto* ri = dynamic_cast<Instr*>(inst);
        if (!ri) return;
        if (!ri->rd.isVreg) out.push_back(ri->rd);
        if (!ri->rs1.isVreg) out.push_back(ri->rs1);
        if (!ri->rs2.isVreg) out.push_back(ri->rs2);

        // 隐式物理寄存器：call / ret
        if (isCall(inst))
        {
            out.push_back(PR::ra);
            pushArgIntRegs(out, ri->call_ireg_cnt);
            pushArgFloatRegs(out, ri->call_freg_cnt);
            out.push_back(PR::a0);
            out.push_back(PR::fa0);
        }
        if (isReturn(inst)) out.push_back(PR::ra);
    }

    void InstrAdapter::insertReloadBefore(
        BE::Block* block, std::deque<BE::MInstruction*>::iterator it, const BE::Register& physReg, int frameIndex) const
    {
        if (!block) return;
        block->insts.insert(it, new BE::FILoadInst(physReg, frameIndex, "reload"));
    }

    void InstrAdapter::insertSpillAfter(
        BE::Block* block, std::deque<BE::MInstruction*>::iterator it, const BE::Register& physReg, int frameIndex) const
    {
        if (!block) return;
        auto nextIt = std::next(it);
        block->insts.insert(nextIt, new BE::FIStoreInst(physReg, frameIndex, "spill"));
    }
}  // namespace BE::Targeting::RV64
