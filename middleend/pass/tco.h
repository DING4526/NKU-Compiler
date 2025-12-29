#ifndef __MIDDLEEND_PASS_TCO_H__
#define __MIDDLEEND_PASS_TCO_H__

#include <interfaces/middleend/pass.h>
#include <middleend/module/ir_function.h>

namespace ME
{
    // Tail Call Optimization (Tail Recursion Elimination)
    //
    // This pass performs a conservative tail-recursion-to-loop transform:
    //   - Only self-recursive calls
    //   - Only when the call is immediately followed by a return of the call result
    //   - Only for scalar parameters that are lowered as allocas in the entry block
    class TCOPass : public FunctionPass
    {
      public:
        TCOPass()  = default;
        ~TCOPass() = default;

        void runOnFunction(Function& function) override;

      private:
        bool runOnFunctionImpl(Function& function);
    };
}  // namespace ME

#endif  // __MIDDLEEND_PASS_TCO_H__
