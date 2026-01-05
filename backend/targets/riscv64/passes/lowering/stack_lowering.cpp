#include <backend/targets/riscv64/passes/lowering/stack_lowering.h>
#include <backend/mir/m_function.h>
#include <backend/mir/m_instruction.h>
#include <backend/mir/m_defs.h>
#include <backend/targets/riscv64/rv64_defs.h>
#include <algorithm>
#include <backend/targets/riscv64/rv64_reg_info.h>
#include <map>
#include <set>

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

        // 收集函数中实际使用到的被调用者保存寄存器（在寄存器分配后运行）
        BE::Targeting::RV64::RegInfo regInfo;
        std::set<int> calleeInt(regInfo.calleeSavedIntRegs().begin(), regInfo.calleeSavedIntRegs().end());
        std::set<int> calleeFloat(regInfo.calleeSavedFloatRegs().begin(), regInfo.calleeSavedFloatRegs().end());
        std::set<int> usedCallee(calleeInt.begin(), calleeInt.end());
        usedCallee.insert(calleeFloat.begin(), calleeFloat.end());

        for (auto& [bid, block] : func->blocks)
        {
            (void)bid;
            if (!block) continue;
            for (auto* inst : block->insts)
            {
                auto* ri = dynamic_cast<BE::RV64::Instr*>(inst);
                if (!ri) continue;

                auto mark = [&](const BE::Register& r) {
                    if (r.isVreg) return;
                    int id = r.rId;
                    if (calleeInt.count(id) || calleeFloat.count(id)) usedCallee.insert(id);
                };
                mark(ri->rd);
                mark(ri->rs1);
                mark(ri->rs2);
            }
        }

        // 始终保存帧指针（x8）以符合 ABI，并为 set_fp 提供原值恢复
        usedCallee.insert(static_cast<int>(BE::RV64::PR::Reg::x8));

        // 为需要保存的寄存器分配栈槽（在计算偏移前）
        std::map<int, int> calleeSpillFI;
        for (int id : usedCallee)
        {
            // 均按 8 字节对齐保存，简化实现
            calleeSpillFI[id] = func->frameInfo.createSpillSlot(8, 8);
        }

        // Assign concrete offsets (bytes from SP after prologue) for all objects.
        func->stackSize = func->frameInfo.calculateOffsets();

        // 获取入口基本块（优先 label 0）
        BE::Block* entry = nullptr;
        if (func->blocks.count(0))
            entry = func->blocks[0];
        else if (!func->blocks.empty())
            entry = func->blocks.begin()->second;

        // 在序言后插入被调用者保存寄存器的保存指令，并建立帧指针
        if (entry)
        {
            auto it = entry->insts.begin();
            // 定位 prologue_sp
            for (; it != entry->insts.end(); ++it)
            {
                auto* ri = dynamic_cast<BE::RV64::Instr*>(*it);
                if (ri && ri->comment == "prologue_sp")
                {
                    ++it;
                    break;
                }
            }
            // 跳过已有的保存（如 ra）
            while (it != entry->insts.end() && dynamic_cast<BE::FIStoreInst*>(*it)) ++it;

            // 设置帧指针：fp = sp（调整后的sp）
            it = entry->insts.insert(it, BE::RV64::createIInst_impl(
                                            BE::RV64::Operator::ADDI, BE::RV64::PR::fp, BE::RV64::PR::sp, 0, "set_fp"));
            ++it;
            
            // Patch栈参数加载指令的偏移
            // 这些指令在frame_lowering中以offset=0插入，现在需要patch为正确的offset
            // 正确的offset是：stackSize + argIndex*8
            for (auto inst_it = entry->insts.begin(); inst_it != entry->insts.end(); ++inst_it)
            {
                auto* ri = dynamic_cast<BE::RV64::Instr*>(*inst_it);
                if (ri && ri->comment.find("stack_arg_load_") == 0)
                {
                    // 从comment中提取argIndex
                    std::string argIndexStr = ri->comment.substr(std::string("stack_arg_load_").length());
                    int argIndex = std::stoi(argIndexStr);
                    
                    // 计算正确的偏移
                    int offset = func->stackSize + argIndex * 8;
                    ri->imme = offset;
                    
                    // 如果偏移超出12位立即数范围，需要特殊处理
                    if (!imm12(offset))
                    {
                        // 替换为多指令序列
                        // li t0, offset; add t0, sp, t0; lw/ld rd, 0(t0)
                        std::vector<BE::MInstruction*> seq;
                        seq.push_back(createUInst(Operator::LI, PR::t0, offset));
                        seq.push_back(createRInst(Operator::ADD, PR::t0, PR::sp, PR::t0));
                        
                        auto* newLoad = createIInst(ri->op, ri->rd, PR::t0, 0);
                        newLoad->comment = ri->comment;
                        seq.push_back(newLoad);
                        
                        // 替换指令
                        BE::MInstruction::delInst(*inst_it);
                        inst_it = entry->insts.erase(inst_it);
                        for (auto* ni : seq)
                        {
                            inst_it = entry->insts.insert(inst_it, ni);
                            ++inst_it;
                        }
                        --inst_it; // 回退
                    }
                }
            }

            // 其余被调用者保存寄存器（除 fp）
            if (!calleeSpillFI.empty())
            {
                // 重新定位插入点：放在 set_fp 之后
                auto insertPos = entry->insts.begin();
                for (; insertPos != entry->insts.end(); ++insertPos)
                {
                    auto* ri = dynamic_cast<BE::RV64::Instr*>(*insertPos);
                    if (ri && ri->comment == "set_fp")
                    {
                        ++insertPos;
                        break;
                    }
                }

                std::vector<int> ordered;
                ordered.reserve(calleeSpillFI.size());
                for (auto& [id, _] : calleeSpillFI) ordered.push_back(id);
                std::sort(ordered.begin(), ordered.end());

                for (int id : ordered)
                {
                    auto* store = new BE::FIStoreInst(BE::RV64::PR::getPR(static_cast<uint32_t>(id)), calleeSpillFI[id],
                        "save_callee");
                    insertPos = entry->insts.insert(insertPos, store);
                    ++insertPos;
                }
            }
        }

        // 在所有返回前插入被调用者保存寄存器的恢复指令，占位为 FILoad，后续统一降解
        if (!calleeSpillFI.empty())
        {
            std::vector<int> ordered;
            ordered.reserve(calleeSpillFI.size());
            for (auto& [id, _] : calleeSpillFI) ordered.push_back(id);
            std::sort(ordered.begin(), ordered.end());

            for (auto& [bid, block] : func->blocks)
            {
                (void)bid;
                if (!block) continue;
                for (auto it = block->insts.begin(); it != block->insts.end(); ++it)
                {
                    auto* ri = dynamic_cast<BE::RV64::Instr*>(*it);
                    if (ri && ri->comment == "epilogue_sp")
                    {
                        for (int id : ordered)
                        {
                            auto* load = new BE::FILoadInst(
                                BE::RV64::PR::getPR(static_cast<uint32_t>(id)), calleeSpillFI[id], "restore_callee");
                            it = block->insts.insert(it, load);
                            ++it;
                        }
                    }
                }
            }
        }

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
                        // 偏移超出立即数范围：生成 li+add 计算地址，再使用 0 偏移访问
                        std::vector<BE::MInstruction*> seq;
                        seq.push_back(createUInst(Operator::LI, PR::t0, off));
                        seq.push_back(createRInst(Operator::ADD, PR::t0, PR::sp, PR::t0));

                        auto isStore = (ri->op == Operator::SW || ri->op == Operator::SD || ri->op == Operator::FSW ||
                                        ri->op == Operator::FSD);
                        if (isStore)
                        {
                            seq.push_back(createSInst(ri->op, ri->rs1, PR::t0, 0));
                        }
                        else
                        {
                            seq.push_back(createIInst(ri->op, ri->rd, PR::t0, 0));
                        }

                        BE::MInstruction::delInst(inst);
                        it = block->insts.erase(it);
                        for (auto* ni : seq)
                        {
                            it = block->insts.insert(it, ni);
                            ++it;
                        }
                        --it;
                    }
                }
            }
        }
        }
    }
}  // namespace BE::RV64::Passes::Lowering
