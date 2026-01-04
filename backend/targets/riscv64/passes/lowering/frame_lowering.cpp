#include "backend/targets/riscv64/rv64_defs.h"
#include <backend/targets/riscv64/passes/lowering/frame_lowering.h>
#include <debug.h>

namespace BE::RV64::Passes::Lowering
{
    void FrameLoweringPass::runOnModule(BE::Module& module)
    {
        for (auto* func : module.functions) runOnFunction(func);
    }

    static bool isCallInst(const BE::MInstruction* inst)
    {
        auto* ri = dynamic_cast<const BE::RV64::Instr*>(inst);
        if (!ri) return false;
        if (ri->op == BE::RV64::Operator::CALL) return true;
        // Allow jal ra, label as call
        if (ri->op == BE::RV64::Operator::JAL && ri->use_label && ri->rd == BE::RV64::PR::ra) return true;
        return false;
    }

    static bool isReturnInst(const BE::MInstruction* inst)
    {
        auto* ri = dynamic_cast<const BE::RV64::Instr*>(inst);
        if (!ri) return false;
        // Common ret form used by isel: jalr x0, ra, 0
        if (ri->op == BE::RV64::Operator::JALR && ri->rd == BE::RV64::PR::x0 && ri->rs1 == BE::RV64::PR::ra &&
            ri->imme == 0)
            return true;
        if (ri->op == BE::RV64::Operator::RET) return true;
        return false;
    }

    void FrameLoweringPass::runOnFunction(BE::Function* func)
    {
        if (!func) return;
        if (func->blocks.empty()) return;

        // Determine entry block (prefer label 0).
        BE::Block* entry = nullptr;
        if (func->blocks.count(0))
            entry = func->blocks[0];
        else
            entry = func->blocks.begin()->second;
        if (!entry) return;

        bool hasCall = false;
        for (auto& [bid, block] : func->blocks)
        {
            (void)bid;
            if (!block) continue;
            for (auto* inst : block->insts)
            {
                if (isCallInst(inst))
                {
                    hasCall = true;
                    break;
                }
            }
            if (hasCall) break;
        }

        int raSaveFI = -1;
        if (hasCall)
        {
            // Reserve a spill slot for saving ra. Offset will be assigned later by StackLowering.
            raSaveFI = func->frameInfo.createSpillSlot(8, 8);
        }

        // Prologue placeholder: StackLowering will patch immediate to -stackSize.
        // We keep it target inst so it prints as real asm.
        entry->insts.push_front(BE::RV64::createIInst_impl(
            BE::RV64::Operator::ADDI, BE::RV64::PR::sp, BE::RV64::PR::sp, 0, "prologue_sp"));

        if (raSaveFI >= 0)
        {
            // Save ra after stack allocation (offsets are based on SP after prologue).
            auto it = entry->insts.begin();
            ++it;  // after prologue_sp
            entry->insts.insert(it, new BE::FIStoreInst(BE::RV64::PR::ra, raSaveFI, "save_ra"));
        }

        // Epilogue: before each return, restore ra (if needed) and deallocate stack.
        for (auto& [bid, block] : func->blocks)
        {
            (void)bid;
            if (!block) continue;

            for (auto it = block->insts.begin(); it != block->insts.end(); ++it)
            {
                auto* inst = *it;
                if (!inst) continue;
                if (!isReturnInst(inst)) continue;

                if (raSaveFI >= 0)
                {
                    it = block->insts.insert(it, new BE::FILoadInst(BE::RV64::PR::ra, raSaveFI, "restore_ra"));
                    ++it;
                }

                it = block->insts.insert(it,
                    BE::RV64::createIInst_impl(
                        BE::RV64::Operator::ADDI, BE::RV64::PR::sp, BE::RV64::PR::sp, 0, "epilogue_sp"));
                ++it;
            }
        }
    }

}  // namespace BE::RV64::Passes::Lowering
