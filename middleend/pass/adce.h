#ifndef __MIDDLEEND_PASS_ADCE_H__
#define __MIDDLEEND_PASS_ADCE_H__

#include "middleend/pass/mem2reg.h"
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