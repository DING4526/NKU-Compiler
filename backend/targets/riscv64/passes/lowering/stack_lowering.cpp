#include <backend/targets/riscv64/passes/lowering/stack_lowering.h>
#include <backend/mir/m_function.h>
#include <backend/mir/m_instruction.h>
#include <backend/mir/m_defs.h>
#include <backend/targets/riscv64/rv64_defs.h>
#include <algorithm>
#include <backend/targets/riscv64/rv64_reg_info.h>

namespace BE::RV64::Passes::Lowering
{
    void StackLoweringPass::runOnModule(BE::Module& module)
    {
        for (auto* func : module.functions) lowerFunction(func);
    }

    static bool imm12(int v) { return v >= -2048 && v <= 2047; }

    static Operator pickLoadOp(const BE::Register& r)
    {
        if (!r.dt) return Operator::LD;
        if (r.dt->dt == BE::DataType::Type::FLOAT)
        {
            return (r.dt->getDataWidth() == 8) ? Operator::FLD : Operator::FLW;
        }
        if (r.dt == BE::I64 || r.dt == BE::PTR || r.dt->getDataWidth() == 8) return Operator::LD;
        return Operator::LW;
    }

    static Operator pickStoreOp(const BE::Register& r)
    {
        if (!r.dt) return Operator::SD;
        if (r.dt->dt == BE::DataType::Type::FLOAT)
        {
            return (r.dt->getDataWidth() == 8) ? Operator::FSD : Operator::FSW;
        }
        if (r.dt == BE::I64 || r.dt == BE::PTR || r.dt->getDataWidth() == 8) return Operator::SD;
        return Operator::SW;
    }

    void StackLoweringPass::lowerFunction(BE::Function* func)
    {
        if (!func) return;
        if (func->blocks.empty()) return;

        // Assign concrete offsets (bytes from SP after prologue) for all objects.
        func->stackSize = func->frameInfo.calculateOffsets();

        for (auto& [bid, block] : func->blocks)
        {
            (void)bid;
            if (!block) continue;

            for (auto it = block->insts.begin(); it != block->insts.end(); ++it)
            {
                auto* inst = *it;
                if (!inst) continue;

                if (auto* ls = dynamic_cast<BE::FILoadInst*>(inst))
                {
                    int off = func->frameInfo.getSpillSlotOffset(ls->frameIndex);
                    ASSERT(off >= 0 && "Invalid spill slot offset");

                    if (!imm12(off))
                    {
                        // Current backend doesn't reserve a dedicated scratch register post-RA.
                        // Keep it simple: require offsets within encodable range.
                        ERROR("Spill slot offset out of imm12 range: %d", off);
                    }

                    Operator op   = pickLoadOp(ls->dest);
                    auto*    real = BE::RV64::createIInst(op, ls->dest, BE::RV64::PR::sp, off);
                    BE::MInstruction::delInst(inst);
                    *it = real;
                    continue;
                }

                if (auto* ss = dynamic_cast<BE::FIStoreInst*>(inst))
                {
                    int off = func->frameInfo.getSpillSlotOffset(ss->frameIndex);
                    ASSERT(off >= 0 && "Invalid spill slot offset");

                    if (!imm12(off))
                    {
                        ERROR("Spill slot offset out of imm12 range: %d", off);
                    }

                    Operator op   = pickStoreOp(ss->src);
                    auto*    real = BE::RV64::createSInst(op, ss->src, BE::RV64::PR::sp, off);
                    BE::MInstruction::delInst(inst);
                    *it = real;
                    continue;
                }

                if (auto* ri = dynamic_cast<BE::RV64::Instr*>(inst))
                {
                    // Patch frame prologue/epilogue stack pointer adjustments.
                    if (ri->op == BE::RV64::Operator::ADDI && ri->rd == BE::RV64::PR::sp && ri->rs1 == BE::RV64::PR::sp)
                    {
                        if (ri->comment == "prologue_sp")
                        {
                            int delta = -func->stackSize;
                            if (!imm12(delta)) ERROR("Stack size out of imm12 range: %d", func->stackSize);
                            ri->imme = delta;
                        }
                        else if (ri->comment == "epilogue_sp")
                        {
                            int delta = func->stackSize;
                            if (!imm12(delta)) ERROR("Stack size out of imm12 range: %d", func->stackSize);
                            ri->imme = delta;
                        }
                    }

                    if (ri->use_ops && ri->fiop && ri->fiop->ot == BE::Operand::Type::FRAME_INDEX)
                    {
                        auto* fiOp = static_cast<BE::FrameIndexOperand*>(ri->fiop);
                        size_t irRegId = static_cast<size_t>(fiOp->frameIndex);
                        int    off     = func->frameInfo.getObjectOffset(irRegId);
                        ASSERT(off >= 0 && "Invalid frame object offset (did you register allocas?)");

                        // Replace abstract FI operand with concrete immediate.
                        delete ri->fiop;
                        ri->fiop    = nullptr;
                        ri->use_ops = false;
                        ri->imme    = off;

                        if (!imm12(off))
                        {
                            // Same as above: require encodable offsets for now.
                            ERROR("Frame object offset out of imm12 range: %d", off);
                        }
                    }
                }
            }
        }
    }
}  // namespace BE::RV64::Passes::Lowering
