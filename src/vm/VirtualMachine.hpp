#ifndef __ULANG_VM_H
#define __ULANG_VM_H

#include <cstddef>

namespace ULang {
    struct HeapBlockHdr {
        size_t size;
        HeapBlockHdr* next;
    };

    class VirtualMachine {
        private:
        bool verbose_en;

        size_t heapsize_start_kb;
        size_t heapsize_limit_kb;
        size_t heapsize_current;
        HeapBlockHdr* heap_start;
        HeapBlockHdr* heap_freelist;

        void heap_init();
        void* heap_alloc(size_t size);
        void heap_free(void* ptr);
        void heap_mergeFree();

        public:
        VirtualMachine(bool verbose_en, size_t heapsize_start_kb, size_t heapsize_limit_kb)
            : verbose_en(verbose_en), heapsize_start_kb(heapsize_start_kb), heapsize_limit_kb(heapsize_limit_kb) {};

        ~VirtualMachine() {};

        void init();
    };
};

#endif
