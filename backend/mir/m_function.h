#ifndef __BACKEND_MIR_M_FUNCTION_H__
#define __BACKEND_MIR_M_FUNCTION_H__

#include <backend/mir/m_block.h>
#include <backend/mir/m_frame_info.h>
#include <map>

namespace BE
{
    // 栈参数信息
    struct StackParamInfo
    {
        Register vreg;        // 目标虚拟寄存器
        int      argIndex;    // 参数索引（第几个栈参数，0-based）
        BE::DataType* dt;     // 数据类型
    };

    class Function
    {
      public:
        std::string                name;
        std::vector<Register>      params;
        std::map<uint32_t, Block*> blocks;

        int                        stackSize     = 0;
        bool                       hasStackParam = false;
        int                        paramSize     = 0;
        std::vector<MInstruction*> allocInsts;
        MFrameInfo                 frameInfo;
        
        // 栈参数信息列表
        std::vector<StackParamInfo> stackParams;

      public:
        Function(const std::string& name)
            : name(name), params(), blocks(), stackSize(0), hasStackParam(false), paramSize(0), frameInfo(), stackParams()
        {}
        ~Function()
        {
            for (auto& [id, block] : blocks)
            {
                delete block;
                block = nullptr;
            }
            blocks.clear();
        }
    };
}  // namespace BE

#endif  // __BACKEND_MIR_M_FUNCTION_H__
