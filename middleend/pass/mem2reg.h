#ifndef __MIDDLEEND_PASS_MEM2REG_H__
#define __MIDDLEEND_PASS_MEM2REG_H__

#include <interfaces/middleend/pass.h>
#include <middleend/module/ir_module.h>
#include <middleend/module/ir_function.h>
#include <middleend/module/ir_block.h>
#include <middleend/module/ir_instruction.h>
#include <middleend/pass/analysis/dominfo.h>
#include <vector>
#include <set>
#include <stack>
#include <map>

namespace ME
{
    class Mem2RegPass : public FunctionPass
    {
      public:
        Mem2RegPass()  = default;
        ~Mem2RegPass() = default;

        void runOnFunction(Function& function) override;

      private:
        void promoteMemoryToRegister(Function& function);
        bool isPromotable(AllocaInst* allocaInst, const std::map<AllocaInst*, std::vector<Instruction*>>& allocaUsers);
        void rename(Block* block);
        void replaceOperands(Instruction* inst);
        void removeDeadCode(Function& function);

        Analysis::DomInfo* domInfo;
        std::map<AllocaInst*, std::stack<Operand*>> stacks;
        std::vector<AllocaInst*> promotableAllocas;
        std::map<size_t, Operand*> regReplacements;
        std::map<PhiInst*, AllocaInst*> phiToAlloca;
        std::set<Instruction*> instsToRemove;
        std::map<size_t, AllocaInst*> promotableRegMap;
        std::map<size_t, Block*> id2block;
        Analysis::CFG* cfg;
    };

}  // namespace ME

#endif  // __MIDDLEEND_PASS_MEM2REG_H__
