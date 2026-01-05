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

        // 插入栈参数加载的占位指令
        // 注意：这里还不知道stackSize，所以插入使用虚拟寄存器的指令
        // 实际的偏移计算将在stack_lowering中完成
        // 但由于我们需要使用sp+stackSize+offset，而stackSize在这里未知
        // 我们插入临时的加载指令，使用一个特殊的标记
        //
        // 更好的方案：使用一个新的伪指令 StackArgLoadPseudoInst
        // 但由于时间限制，我们直接在这里生成真实指令，使用占位offset
        // stack_lowering将patch这些指令
        if (func->hasStackParam && !func->stackParams.empty())
        {
            // 在prologue之后插入栈参数加载
            // 这些指令使用虚拟寄存器，将由RA处理
            auto it = entry->insts.begin();
            ++it; // skip prologue_sp
            
            for (const auto& paramInfo : func->stackParams)
            {
                // 创建一个临时的加载指令
                // 使用comment标记为需要patch偏移的指令
                BE::RV64::Operator loadOp;
                if (paramInfo.dt == BE::F32)
                    loadOp = BE::RV64::Operator::FLW;
                else if (paramInfo.dt == BE::F64)
                    loadOp = BE::RV64::Operator::FLD;
                else if (paramInfo.dt == BE::I64 || paramInfo.dt == BE::PTR)
                    loadOp = BE::RV64::Operator::LD;
                else
                    loadOp = BE::RV64::Operator::LW;
                
                // 使用占位offset=0，并标记comment，stack_lowering将patch它
                auto* loadInst = BE::RV64::createIInst_impl(loadOp, paramInfo.vreg, BE::RV64::PR::sp, 0,
                    "stack_arg_load_" + std::to_string(paramInfo.argIndex));
                it = entry->insts.insert(it, loadInst);
                ++it;
            }
        }

        if (raSaveFI >= 0)
        {
            // Save ra after stack allocation (offsets are based on SP after prologue).
            auto it = entry->insts.begin();
            ++it;  // after prologue_sp
            // Skip stack param loads
            while (it != entry->insts.end())
            {
                auto* ri = dynamic_cast<BE::RV64::Instr*>(*it);
                if (ri && ri->comment.find("stack_arg_load_") == 0)
                {
                    ++it;
                    continue;
                }
                break;
            }
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
