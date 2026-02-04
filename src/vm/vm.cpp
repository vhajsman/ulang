#include "VirtualMachine.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>

namespace ULang {
    void VirtualMachine::init() {
        if(this->verbose_en) {
            std::cout << "ULang VM v 1.0. - https://github.com/vhajsman/ulang" << std::endl;
            std::cout << "INIT: register size: " << sizeof(uint64_t) << ", count: " << this->REG_COUNT << std::endl;
            std::cout << "INIT: total register-occupied memory: " << sizeof(uint64_t) * this->REG_COUNT << std::endl;
            std::cout << "INIT: stack size: " << STACK_SIZE << std::endl;
        }

        memset(this->regs, 0x00, sizeof(uint64_t) * this->REG_COUNT);
        this->pc =    &regs[R_PC.reg_no];       // Program counter
        this->sp =    &regs[R_SP.reg_no];       // Stack pointer
        this->fp =    &regs[R_FP.reg_no];       // Frame pointer
        this->flags = &regs[R_FLAGS.reg_no];    // Flags

        this->stack = (uint8_t*) malloc(this->STACK_SIZE);
        *this->sp = reinterpret_cast<uint64_t>(this->stack + this->STACK_SIZE);

        this->heap_init();
    }
};