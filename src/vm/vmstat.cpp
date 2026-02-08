#include "vmstat.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>

namespace ULang {
    void StatTimeMeter::start() {
        this->time_start = std::chrono::high_resolution_clock::now();
        this->running = true;
    }

    void StatTimeMeter::stop() {
        if(!this->running)
            return;

        this->time_end = std::chrono::high_resolution_clock::now();
        this->running = false;
    }

    void StatTimeMeter::report(std::string& meter_name) const {
        std::cout << "VMSTAT: TIME: " << meter_name << ": elapsed: " << this->microseconds() << " us" << std::endl;
    }

    uint64_t StatTimeMeter::microseconds() const {
        std::chrono::high_resolution_clock::time_point tp = !this->running ?
            this->time_end : std::chrono::high_resolution_clock::now();

        return std::chrono::duration_cast<std::chrono::microseconds>(tp - this->time_start).count();
    }

    void StatTimeMeter::reset() {
        this->running = false;
    }

    template<typename T_RET, typename T_ARG> 
    void StatTimeMeter::attach(T_RET(*caller)(T_ARG), T_ARG caller_arg) {
        this->start();
        (void) caller(caller_arg);
        this->stop();
    }

    template<typename T_RET, typename T_ARG> 
    void StatTimeMeter::attach(T_RET(*caller)(T_ARG), T_ARG caller_arg, void(*then)(T_RET, StatTimeMeter&)) {
        this->start();
        T_RET caller_ret = caller(caller_arg);

        this->stop();
        then(caller_ret, this);
    }



    void StatMemoryMeter::record_alloc(uint64_t size) {
        if(size + this->allocated_curr > this->allocated_max)   this->allocated_max += size;
        if(size > this->allocated_largest)                      this->allocated_largest = size;
        if(size < this->allocated_smallest)                     this->allocated_smallest = size;

        this->allocated_avg = (this->allocated_avg + size) / 2;
        this->allocated_tot += size;
        this->allocated_curr += size;

        this->allocation_count++;
    }

    void StatMemoryMeter::record_free(uint64_t size) {
        this->allocated_curr -= size;

        this->free_count++;
        this->freed_tot += size;
    }

    void StatMemoryMeter::report(std::string& meter_name) const {
        std::cout << "VMSTAT: MEM: " << meter_name << ": " << std::endl;

        std::cout << "VMSTAT: MEM:    --> Maximal area ever allocated in total: "   << this->allocated_max << std::endl;
        std::cout << "VMSTAT: MEM:    --> Largest area ever allocated: "            << this->allocated_largest << std::endl;
        std::cout << "VMSTAT: MEM:    --> Smallest area ever allocated: "           << this->allocated_smallest << std::endl;
        std::cout << "VMSTAT: MEM:    --> Average allocation size: "                << this->allocated_avg << std::endl;
        std::cout << "VMSTAT: MEM:    --> Total allocated area: "                   << this->allocated_tot << std::endl;
        std::cout << "VMSTAT: MEM:    --> Currently allocated area: "               << this->allocated_curr << std::endl;
        std::cout << "VMSTAT: MEM:    --> Total allocation count: "                 << this->allocation_count << std::endl;

        std::cout << "VMSTAT: MEM:    --> Totally freed memory: "                   << this->freed_tot << std::endl;
        std::cout << "VMSTAT: MEM:    --> Free count: "                             << this->free_count << std::endl;
    }

    void StatMemoryMeter::reset() {
        this->allocated_max      = 0;
        this->allocated_largest  = 0;
        this->allocated_smallest = 0;
        this->allocated_avg      = 0;
        this->allocated_tot      = 0;
        this->freed_tot          = 0;
        this->allocated_curr     = 0;
        this->allocation_count   = 0;
        this->free_count         = 0;
    }

    template<typename T_RET, typename T_ARG> 
    void StatMemoryMeter::attach(T_RET(*caller)(T_ARG), T_ARG caller_arg) {
        this->reset();
        (void) caller(caller_arg);
    }

    template<typename T_RET, typename T_ARG> 
    void StatMemoryMeter::attach(T_RET(*caller)(T_ARG), T_ARG caller_arg, void(*then)(T_RET, StatMemoryMeter&)) {
        this->reset();
        T_RET caller_ret = caller(caller_arg);
        then(caller_ret);
    }



    void StatUCounterMeter::report(std::string& meter_name) const {
        std::cout << "VMSTAT: " << meter_name << " = " << this->count << std::endl;
    }

    void StatUCounterMeter::reset() {
        this->count = 0;
    }

    template<typename T_RET, typename T_ARG> 
    void StatUCounterMeter::attach(T_RET(*caller)(T_ARG), T_ARG caller_arg) {
        this->reset();
        (void) caller(caller_arg);
    }

    template<typename T_RET, typename T_ARG> 
    void StatUCounterMeter::attach(T_RET(*caller)(T_ARG), T_ARG caller_arg, void(*then)(T_RET, StatUCounterMeter&)) {
        this->reset();
        T_RET caller_ret = caller(caller_arg);
        then(caller_ret);
    }

    void StatUCounterMeter::add(uint64_t val) {this->count += val;}
    void StatUCounterMeter::sub(uint64_t val) {this->count -= val;}
    void StatUCounterMeter::set(uint64_t val) {this->count = val;}
}