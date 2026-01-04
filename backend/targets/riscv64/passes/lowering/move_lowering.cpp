#include <backend/targets/riscv64/passes/lowering/move_lowering.h>

#include <backend/mir/m_defs.h>
#include <backend/mir/m_function.h>
#include <backend/mir/m_instruction.h>
#include <backend/targets/riscv64/rv64_defs.h>
#include <debug.h>
#include <transfer.h>

namespace BE::RV64::Passes::Lowering
{
    using namespace BE;
    using namespace BE::RV64;

    void MoveLoweringPass::runOnModule(BE::Module& module)
    {
        for (auto* func : module.functions) lowerFunction(func);
    }

    static Operator pickIntMoveOp(const BE::Register& dst)
    {
        if (dst.dt == BE::I32) return Operator::ADDIW;
        return Operator::ADDI;
    }

    static Operator pickFMoveOp(const BE::Register& dst)
    {
        if (dst.dt == BE::F64) return Operator::FMV_D;
        return Operator::FMV_S;
    }

    void MoveLoweringPass::lowerFunction(BE::Function* func)
    {
        if (!func) return;

        for (auto& [bid, block] : func->blocks)
        {
            (void)bid;
            if (!block) continue;

            for (auto it = block->insts.begin(); it != block->insts.end();)
            {
                auto* mv = dynamic_cast<BE::MoveInst*>(*it);
                if (!mv)
                {
                    ++it;
                    continue;
                }

                auto* dstOp = dynamic_cast<BE::RegOperand*>(mv->dest);
                ASSERT(dstOp && "MoveLowering expects RegOperand destination");

                BE::Register dst = dstOp->reg;

                // Replace this one MoveInst with 1..2 real RV64 instructions.
                std::vector<BE::MInstruction*> lowered;

                if (auto* srcR = dynamic_cast<BE::RegOperand*>(mv->src))
                {
                    BE::Register src = srcR->reg;
                    if (dst.dt && dst.dt->dt == BE::DataType::Type::FLOAT)
                    {
                        lowered.push_back(createR2Inst(pickFMoveOp(dst), dst, src));
                    }
                    else
                    {
                        lowered.push_back(createIInst(pickIntMoveOp(dst), dst, src, 0));
                    }
                }
                else if (auto* imm = dynamic_cast<BE::I32Operand*>(mv->src))
                {
                    // Use LI pseudo (assembler accepts), keeps immediate range broad.
                    lowered.push_back(createUInst(Operator::LI, dst, imm->val));
                }
                else if (auto* fimm = dynamic_cast<BE::F32Operand*>(mv->src))
                {
                    // Materialize float immediate: li tmp, bits; fmv.w.x dst, tmp
                    int bits = FLOAT_TO_INT_BITS(fimm->val);
                    BE::Register tmp = BE::getVReg(BE::I64);
                    lowered.push_back(createUInst(Operator::LI, tmp, bits));
                    lowered.push_back(createR2Inst(Operator::FMV_W_X, dst, tmp));
                }
                else
                {
                    ERROR("Unsupported MOVE operand kind in RV64 move lowering");
                }

                // Splice lowered instructions in place.
                auto eraseIt = it;
                ++it;
                BE::MInstruction::delInst(*eraseIt);
                eraseIt = block->insts.erase(eraseIt);

                for (auto* ni : lowered)
                {
                    eraseIt = block->insts.insert(eraseIt, ni);
                    ++eraseIt;
                }
            }
        }
    }
}  // namespace BE::RV64::Passes::Lowering
