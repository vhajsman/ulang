#ifndef __ULANG_VM_H
#define __ULANG_VM_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "bytecode.hpp"
#include "vmreg_defines.hpp"

namespace ULang {
    struct HeapBlockHdr {
        size_t size;
        HeapBlockHdr* next;
    };

    class VirtualMachine {
        private:
        bool verbose_en;

        // ==================================================================
        // ======== MEMORY MANAGEMENT
        // ==================================================================

        size_t heapsize_start_kb;
        size_t heapsize_limit_kb;
        size_t heapsize_current;
        HeapBlockHdr* heap_start;
        HeapBlockHdr* heap_freelist;
        
        void heap_init();
        void* heap_alloc(size_t size);
        void heap_free(void* ptr);
        void heap_mergeFree();

        // ==================================================================
        // ======== REGISTERS
        // ==================================================================

        static constexpr uint32_t REG_COUNT = 32;
        uint64_t regs[REG_COUNT];

        // ==================================================================
        // ======== VM STATE + STACK
        // ==================================================================

        uint64_t* pc; uint64_t* sp; uint64_t* fp; uint64_t* flags;

        static constexpr size_t STACK_SIZE = 256 * 1024;
        uint8_t* stack;

        // ==================================================================
        // ======== EXECUTION
        // ==================================================================

        void execute(const Instruction& instr);

        public:
        VirtualMachine(bool verbose_en, size_t heapsize_start_kb, size_t heapsize_limit_kb)
            : verbose_en(verbose_en), heapsize_start_kb(heapsize_start_kb), heapsize_limit_kb(heapsize_limit_kb) {};

        ~VirtualMachine() {
            free(this->stack);
        };

        void init();
        void run(const std::vector<Instruction> program);
        void halt();
    };
};

#endif
