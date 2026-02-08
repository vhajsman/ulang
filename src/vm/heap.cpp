#include "VirtualMachine.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace ULang {
    void VirtualMachine::heap_init() {
        if(this->vmparams.verbose_en) {
            std::cout << "HEAP: Heap initialization" << std::endl;
            std::cout << "HEAP: Heap size starting: " << this->vmparams.heapsize_start_kb << "K, max: " << this->vmparams.heapsize_limit_kb << "K" << std::endl;
        }

        size_t bytes = this->vmparams.heapsize_start_kb * 1024;

        this->heap_base = reinterpret_cast<uint8_t*>(malloc(bytes));
        if(!this->heap_base)
            throw std::runtime_error("Could not allocate starting block for heap");
        
        this->heap_start = reinterpret_cast<HeapBlockHdr*>(this->heap_base);
        this->heap_start->size = bytes - sizeof(HeapBlockHdr);
        this->heap_start->next = nullptr;

        this->heap_freelist = heap_start;
        this->heapsize_current = sizeof(HeapBlockHdr);
    }

    void* VirtualMachine::heap_alloc(size_t size) {
        if(this->vmparams.heapsize_limit_kb != 0 && (this->heapsize_current + size + sizeof(HeapBlockHdr)) * 1024 > this->vmparams.heapsize_limit_kb)
            throw std::runtime_error("insufficent resources");

        HeapBlockHdr* current = this->heap_freelist;
        while(current) {
            if(current->size >= size + sizeof(HeapBlockHdr)) {
                HeapBlockHdr* blk_new = (HeapBlockHdr*)((char*)current + sizeof(HeapBlockHdr) + size);
                blk_new->size = current->size - size - sizeof(HeapBlockHdr);
                blk_new->next = current->next;

                current->size = size;
                current->next = blk_new;

                this->heapsize_current += blk_new->size;
                void* result = (void*)((char*)current + sizeof(HeapBlockHdr));

                if(this->vmparams.verbose_en) {
                    std::cout << "HEAP: alloc: " << size << ", now occupied " << this->heapsize_current << std::endl;
                    std::cout << "HEAP:   --> addr: " << result << std::endl;
                }

                return result;
            }

            current = current->next;
        }

        throw std::runtime_error("insufficent resources");
    }

    void VirtualMachine::heap_free(void* ptr) {
        if(!ptr) {
            if(this->vmparams.verbose_en) 
                std::cout << "HEAP: free: ptr=null" << std::endl;

            return;
        }

        HeapBlockHdr* target = (HeapBlockHdr*)((char*)ptr - sizeof(HeapBlockHdr));
        HeapBlockHdr* current = this->heap_freelist;

        target->next = current;
        this->heap_freelist = target->next;

        if(this->vmparams.verbose_en)
            std::cout << "HEAP: free: " << &target;

        this->heap_mergeFree();
    }

    void VirtualMachine::heap_mergeFree() {
        HeapBlockHdr* current = this->heap_freelist;
        while(current && current->next) {
            if((char*)current + sizeof(HeapBlockHdr) + current->size == (char*)current->next) {
                current->size += sizeof(HeapBlockHdr) + current->next->size;
                current->next = current->next->next;
                continue;
            }

            current = current->next;
        }
    }

    uint8_t* VirtualMachine::castHeapReference(uint64_t offset) {
        if(offset >= this->heapsize_current)
            throw std::runtime_error("Heap reference out of bounds");

        return this->heap_base + offset;
    }
};