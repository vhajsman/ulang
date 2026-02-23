#include "bytecode.hpp"
#include "vmreg_defines.hpp"
#include "compiler.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#define cout_verbose        \
    if(this->cparams.verbose)    \
        std::cout

namespace ULang {
    Operand CompilerInstance::allocTmpReg() {
        for(uint32_t i = 0; i < this->tmp_used.size(); i++) {
            if(!this->tmp_used[i]) {
                this->tmp_used[i] = true;
                this->verbose_nl("* allocTmpReg -> TMP" + std::to_string(i), true);
                return {OperandType::OP_REGISTER, R_TMP0.reg_no + i};
            }
        }

        throw std::runtime_error("No free tmp register available");
    }

    void CompilerInstance::freeTmpReg(uint8_t reg, bool failsafe) {
        if(reg >= this->tmp_used.size()) {
            if(failsafe) {
                this->verbose_nl("* FAILSAFE freeTmpReg TMP" + std::to_string(reg), true);
                return;
            };

            throw std::runtime_error("Invalid tmp register free");
        }

        this->tmp_used[reg] = false;
        this->verbose_nl("* OK freeTmpReg TMP" + std::to_string(reg), true);
    }

    void CompilerInstance::freeTmpReg(Operand reg, bool failsafe) {
        if(reg.type != OperandType::OP_REGISTER || reg.data < R_TMP0.reg_no || reg.data >= R_TMP0.reg_no + this->tmp_used.size()) {
            if(failsafe) {
                this->verbose_nl("* FAILSAFE freeTmpReg TMP" + std::to_string(reg.data - R_TMP0.reg_no), true);
                return;
            };

            throw std::runtime_error("Invalid tmp register free");
        }

        this->tmp_used[reg.data - R_TMP0.reg_no] = false;
        this->verbose_nl("* OK freeTmpReg TMP" + std::to_string(reg.data - R_TMP0.reg_no), true);
    }
};

#undef cout_verbose
