#ifndef __MIDDLEEND_PASS_ADCE_H__
#define __MIDDLEEND_PASS_ADCE_H__

#include <interfaces/middleend/pass.h>
#include <middleend/module/ir_module.h>
#include <middleend/module/ir_function.h>
#include <middleend/module/ir_block.h>
#include <middleend/module/ir_instruction.h>
#include <middleend/pass/analysis/dominfo.h>

namespace ME {
    class ADCEPass : public FunctionPass {
        public:
            ADCEPass() = default;
            ~ADCEPass() = default;
            void runOnFunction(Function& function) override;
        private:

    } ;
} ;

#endif