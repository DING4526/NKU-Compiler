#ifndef __MIDDLEEND_PASS_LICM_H__
#define __MIDDLEEND_PASS_LICM_H__

#include <interfaces/middleend/pass.h>
#include <middleend/module/ir_function.h>

namespace ME
{
    // 标量 LICM：将循环内的标量循环不变量指令外提到 preheader。
    // 该实现偏保守：只外提无副作用且“可推测执行”的指令，并要求结果仅在循环内使用。
    class ScalarLICMPass : public FunctionPass
    {
      public:
        ScalarLICMPass()  = default;
        ~ScalarLICMPass() = default;

        void runOnFunction(Function& function) override;

      private:
        void runScalarLICM(Function& function);
    };
}  // namespace ME

#endif  // __MIDDLEEND_PASS_LICM_H__
