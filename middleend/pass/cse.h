#ifndef __MIDDLEEND_PASS_CSE_H__
#define __MIDDLEEND_PASS_CSE_H__

#include <interfaces/middleend/pass.h>
#include <middleend/module/ir_module.h>
#include <middleend/module/ir_function.h>
#include <middleend/module/ir_block.h>
#include <middleend/module/ir_instruction.h>
#include <vector>
#include <set>
#include <map>
#include <tuple>
#include <string>

namespace ME
{
    class CSEPass : public FunctionPass
    {
      public:
        CSEPass()  = default;
        ~CSEPass() = default;

        void runOnFunction(Function& function) override;

      private:
        // Helper to convert operand to a comparable key string
        std::string operandKey(Operand* op) const;

        // Generate expression key for CSE matching
        std::string makeExprKey(Instruction* inst) const;

        // Check if an instruction is a commutative operator
        bool isCommutative(Operator op) const;

        // Check if an instruction is CSE-able (pure computation)
        bool isCSEableInst(Instruction* inst) const;

        // Replace operands in an instruction with their replacements
        void replaceOperands(Instruction* inst);

        // Get the result operand of an instruction (if any)
        Operand* getInstructionResult(Instruction* inst) const;

        // Compare two operands for ordering (returns true if a < b)
        bool operandLessThan(Operand* a, Operand* b) const;
        
        // Maps for CSE
        std::map<std::string, Operand*> expr2value;  // expression key -> computed result
        std::map<size_t, Operand*> regRepl;          // old reg -> replacement operand
        std::set<Instruction*> instsToRemove;
        std::map<size_t, std::set<size_t>> regUseBlocks;  // reg -> blocks where this reg is USED (including phi incoming)
        void buildUseBlocks(Function& function);
        bool resultEscapesBlock(size_t reg, size_t curBlockId) const;
    };

}  // namespace ME

#endif  // __MIDDLEEND_PASS_CSE_H__
