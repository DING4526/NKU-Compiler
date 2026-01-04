#ifndef __BACKEND_RV64_PASSES_LOWERING_MOVE_LOWERING_H__
#define __BACKEND_RV64_PASSES_LOWERING_MOVE_LOWERING_H__

#include <backend/mir/m_module.h>

namespace BE::RV64::Passes::Lowering
{
    class MoveLoweringPass
    {
      public:
        MoveLoweringPass()  = default;
        ~MoveLoweringPass() = default;

        void runOnModule(BE::Module& module);

      private:
        void lowerFunction(BE::Function* func);
    };
}  // namespace BE::RV64::Passes::Lowering

#endif  // __BACKEND_RV64_PASSES_LOWERING_MOVE_LOWERING_H__
