#ifndef __ULANG_VM_H
#define __ULANG_VM_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
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
        // ======== STATS
        // ==================================================================

        std::chrono::steady_clock::time_point stat_exec_begin;

        // ==================================================================
        // ======== MEMORY MANAGEMENT
        // ==================================================================

        size_t heapsize_start_kb;       ///< Size in kilobytes (1024) heap allocates at begining
        size_t heapsize_limit_kb;       ///< Maximal size in kilobytes (1024) heap can allocate
        size_t heapsize_current;        ///< Current heap size in bytes
        HeapBlockHdr* heap_start;       ///< Heap start pointer
        HeapBlockHdr* heap_freelist;    ///< Heap free list pointer (cyclist)

        uint8_t* heap_base;             ///< Heap memory pool pointer
        
        /**
         * @brief Initializes the heap
         * @exception std::runtime-error when allocation error
         */
        void heap_init();

        /**
         * @brief Allocates the area in the memory pool
         * @exception std::runtime-error when allocation fails
         * @param size desired size in bytes
         * @return void* allocated memory area pointer
         */
        void* heap_alloc(size_t size);

        /**
         * @brief Deallocates the area in memory pool
         * @param ptr pointer to allocated memory area
         */
        void heap_free(void* ptr);

        /**
         * @brief Merges all the free blocks in free list
         */
        void heap_mergeFree();

        /**
         * @brief Converts virtual memory offset to real memory pointer
         * @exception std::runtime_error if heap reference out of bounds
         * @param offset offset in virtual memory
         * @return uint8_t* real memory pointer
         */
        uint8_t* castHeapReference(uint64_t offset);

        // ==================================================================
        // ======== REGISTERS
        // ==================================================================

        static constexpr uint32_t REG_COUNT = 32;
        uint64_t regs[REG_COUNT];   ///< register file

        // ==================================================================
        // ======== VM STATE + STACK
        // ==================================================================

        uint64_t* pc;       ///< program counter register pointer
        uint64_t* sp;       ///< stack top pointer register pointer
        uint64_t* fp;       ///< frame pointer register pointer
        uint64_t* flags;    ///< execution flags register pointer

        static constexpr size_t STACK_SIZE = 256 * 1024;
        uint8_t* stack;

        // ==================================================================
        // ======== EXECUTION
        // ==================================================================

        /**
         * @brief Executes signle instruction from the program
         * @exception std::runtime_error
         * @param instr instruction structure
         */
        void execute(const Instruction& instr);

        /**
         * @brief Reads operand value
         * @exception std::runtime_error when attempting to read invalid operand type
         * @param op operand structure
         * @return uint64_t operand value
         */
        uint64_t readOpCast(const Operand& op);

        /**
         * @brief Writes the operand value
         * @exception std::runtime_error when attempting to write readonly operand type
         * @exception std::runtime_error when attempting to write invalid operand type
         * @param op operand structure
         * @param val desired value
         */
        void writeOpCast(const Operand& op, uint64_t val);

        public:
        VirtualMachine(bool verbose_en, size_t heapsize_start_kb, size_t heapsize_limit_kb)
            : verbose_en(verbose_en), heapsize_start_kb(heapsize_start_kb), heapsize_limit_kb(heapsize_limit_kb) {};

        ~VirtualMachine() {
            free(this->stack); this->stack = nullptr;
            free(this->heap_base); this->heap_base = nullptr;

            if(this->verbose_en)
                std::cout << "VM:DESTRUCTOR: memory freed" << std::endl;
        };

        void init();
        void run(const std::vector<Instruction> program);
        void halt();
    };
};

#endif
