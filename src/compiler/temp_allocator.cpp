#include "bytecode.hpp"
#include "vmreg_defines.hpp"
#include "compiler.hpp"
#include <cstdint>
#include <stdexcept>

namespace ULang {
    Operand CompilerInstance::allocTmpReg() {
        for(uint32_t i = 0; i < this->tmp_used.size(); i++) {
            if(!this->tmp_used[i]) {
                this->tmp_used[i] = true;
                return {OperandType::OP_REGISTER, R_TMP0.reg_no + i};
            }
        }

        throw std::runtime_error("No free tmp register available");
    }

    void CompilerInstance::freeTmpReg(uint8_t reg) {
        if(reg >= this->tmp_used.size())
            throw std::runtime_error("Invalid tmp register free");

        this->tmp_used[reg] = false;
    }

    void CompilerInstance::freeTmpReg(Operand reg) {
        if(reg.type != OperandType::OP_REGISTER || reg.data < R_TMP0.reg_no || reg.data >= R_TMP0.reg_no + this->tmp_used.size())
            throw std::runtime_error("Invalid tmp register free");

        this->tmp_used[reg.data - R_TMP0.reg_no] = false;
    }
};
