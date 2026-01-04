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

    // 辅助函数：生成栈指针调整指令序列
    // 当偏移量超过 12 位立即数范围时，使用 LI + ADD 序列
    static std::vector<BE::MInstruction*> generateSPAdjust(int delta, const std::string& comment)
    {
        std::vector<BE::MInstruction*> insts;
        if (imm12(delta))
        {
            auto* inst = createIInst(Operator::ADDI, PR::sp, PR::sp, delta);
            inst->comment = comment;
            insts.push_back(inst);
        }
        else
        {
            // 使用 t0 作为临时寄存器加载大立即数
            insts.push_back(createUInst(Operator::LI, PR::t0, delta));
            auto* addInst = createRInst(Operator::ADD, PR::sp, PR::sp, PR::t0);
            addInst->comment = comment;
            insts.push_back(addInst);
        }
        return insts;
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

                    Operator op = pickLoadOp(ls->dest);
                    
                    if (imm12(off))
                    {
                        auto* real = BE::RV64::createIInst(op, ls->dest, BE::RV64::PR::sp, off);
                        BE::MInstruction::delInst(inst);
                        *it = real;
                    }
                    else
                    {
                        // 偏移超范围：使用 t0 临时寄存器计算地址
                        // li t0, off; add t0, sp, t0; lw/ld dest, 0(t0)
                        std::vector<BE::MInstruction*> seq;
                        seq.push_back(createUInst(Operator::LI, PR::t0, off));
                        seq.push_back(createRInst(Operator::ADD, PR::t0, PR::sp, PR::t0));
                        seq.push_back(createIInst(op, ls->dest, PR::t0, 0));
                        
                        BE::MInstruction::delInst(inst);
                        it = block->insts.erase(it);
                        for (auto* ni : seq)
                        {
                            it = block->insts.insert(it, ni);
                            ++it;
                        }
                        --it;
                    }
                    continue;
                }

                if (auto* ss = dynamic_cast<BE::FIStoreInst*>(inst))
                {
                    int off = func->frameInfo.getSpillSlotOffset(ss->frameIndex);
                    ASSERT(off >= 0 && "Invalid spill slot offset");

                    Operator op = pickStoreOp(ss->src);
                    
                    if (imm12(off))
                    {
                        auto* real = BE::RV64::createSInst(op, ss->src, BE::RV64::PR::sp, off);
                        BE::MInstruction::delInst(inst);
                        *it = real;
                    }
                    else
                    {
                        // 偏移超范围：使用 t0 临时寄存器计算地址
                        // li t0, off; add t0, sp, t0; sw/sd src, 0(t0)
                        std::vector<BE::MInstruction*> seq;
                        seq.push_back(createUInst(Operator::LI, PR::t0, off));
                        seq.push_back(createRInst(Operator::ADD, PR::t0, PR::sp, PR::t0));
                        seq.push_back(createSInst(op, ss->src, PR::t0, 0));
                        
                        BE::MInstruction::delInst(inst);
                        it = block->insts.erase(it);
                        for (auto* ni : seq)
                        {
                            it = block->insts.insert(it, ni);
                            ++it;
                        }
                        --it;
                    }
                    continue;
                }

                if (auto* ri = dynamic_cast<BE::RV64::Instr*>(inst))
                {
                    // Patch frame prologue/epilogue stack pointer adjustments.
                    // 当栈大小超过 12 位立即数范围时，需要替换为多条指令
                    if (ri->op == BE::RV64::Operator::ADDI && ri->rd == BE::RV64::PR::sp && ri->rs1 == BE::RV64::PR::sp)
                    {
                        if (ri->comment == "prologue_sp")
                        {
                            int delta = -func->stackSize;
                            if (imm12(delta))
                            {
                                ri->imme = delta;
                            }
                            else
                            {
                                // 替换为 LI + ADD 序列
                                auto newInsts = generateSPAdjust(delta, "prologue_sp");
                                BE::MInstruction::delInst(inst);
                                it = block->insts.erase(it);
                                for (auto* ni : newInsts)
                                {
                                    it = block->insts.insert(it, ni);
                                    ++it;
                                }
                                --it;  // 回退以便下次迭代正确
                                continue;
                            }
                        }
                        else if (ri->comment == "epilogue_sp")
                        {
                            int delta = func->stackSize;
                            if (imm12(delta))
                            {
                                ri->imme = delta;
                            }
                            else
                            {
                                // 替换为 LI + ADD 序列
                                auto newInsts = generateSPAdjust(delta, "epilogue_sp");
                                BE::MInstruction::delInst(inst);
                                it = block->insts.erase(it);
                                for (auto* ni : newInsts)
                                {
                                    it = block->insts.insert(it, ni);
                                    ++it;
                                }
                                --it;  // 回退以便下次迭代正确
                                continue;
                            }
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
