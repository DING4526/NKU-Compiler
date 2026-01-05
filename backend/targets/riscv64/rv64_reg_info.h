#ifndef __BACKEND_TARGETS_RISCV64_RV64_REG_INFO_H__
#define __BACKEND_TARGETS_RISCV64_RV64_REG_INFO_H__

#include <backend/target/target_reg_info.h>
#include <backend/targets/riscv64/rv64_defs.h>
#include <array>
#include <map>

namespace BE::Targeting::RV64
{
    class RegInfo : public TargetRegInfo
    {
      public:
        RegInfo()
        {
            // Physical register IDs must match BE::Register::rId.
            // RV64 mapping in rv64_defs.cpp: x0..x31 => 0..31, f0..f31 => 32..63.

            sp_   = static_cast<int>(BE::RV64::PR::Reg::x2);  // sp
            ra_   = static_cast<int>(BE::RV64::PR::Reg::x1);  // ra
            zero_ = static_cast<int>(BE::RV64::PR::Reg::x0);  // x0

            // ABI arg registers: a0-a7, fa0-fa7
            intArgRegs_   = {10, 11, 12, 13, 14, 15, 16, 17};
            floatArgRegs_ = {42, 43, 44, 45, 46, 47, 48, 49};

            // Callee-saved: s0-s11, fs0-fs11
            calleeSavedInt_   = {8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};
            calleeSavedFloat_ = {40, 41, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59};

            // 保留零/ra/sp/gp/tp/t0 以及专用的 fp(x8)。
            // ra 不能分配虚拟寄存器，t0 在多处作为固定临时使用，避免后续降级时误覆盖真实值。
            reserved_ = {0, 1, 2, 3, 4, 5, 8};

            // All integer regs (x0-x31)
            intRegs_.clear();
            for (int i = 0; i < 32; ++i) intRegs_.push_back(i);

            // All float regs (f0-f31) => 32..63
            floatRegs_.clear();
            for (int i = 32; i < 64; ++i) floatRegs_.push_back(i);
        }

        int spRegId() const override { return sp_; }
        int raRegId() const override { return ra_; }
        int zeroRegId() const override { return zero_; }

        const std::vector<int>& intArgRegs() const override { return intArgRegs_; }
        const std::vector<int>& floatArgRegs() const override { return floatArgRegs_; }

        const std::vector<int>& calleeSavedIntRegs() const override { return calleeSavedInt_; }
        const std::vector<int>& calleeSavedFloatRegs() const override { return calleeSavedFloat_; }

        const std::vector<int>& reservedRegs() const override { return reserved_; }
        const std::vector<int>& intRegs() const override { return intRegs_; }
        const std::vector<int>& floatRegs() const override { return floatRegs_; }

      private:
        int sp_   = 2;
        int ra_   = 1;
        int zero_ = 0;

        std::vector<int> intArgRegs_;
        std::vector<int> floatArgRegs_;

        std::vector<int> calleeSavedInt_;
        std::vector<int> calleeSavedFloat_;

        std::vector<int> reserved_;
        std::vector<int> intRegs_;
        std::vector<int> floatRegs_;
    };
}  // namespace BE::Targeting::RV64

#endif  // __BACKEND_TARGETS_RISCV64_RV64_REG_INFO_H__
