#ifndef __ULANG_VMSTAT_H
#define __ULANG_VMSTAT_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
namespace ULang {
    class StatTimeMeter {
        private:
        std::chrono::high_resolution_clock::time_point time_start;
        std::chrono::high_resolution_clock::time_point time_end;
        bool running = false;

        public:
        void start();
        void stop();
        void reset();

        void report(std::string& meter_name) const;
        
        template<typename T_RET, typename T_ARG> void attach(T_RET(*caller)(T_ARG), T_ARG caller_arg);
        template<typename T_RET, typename T_ARG> void attach(T_RET(*caller)(T_ARG), T_ARG caller_arg, void(*then)(T_RET, StatTimeMeter&));

        uint64_t microseconds() const;
    };

    class StatMemoryMeter {
        private:
        uint64_t allocated_max;
        uint64_t allocated_largest;
        uint64_t allocated_smallest;
        uint64_t allocated_avg;
        uint64_t allocated_tot;
        uint64_t freed_tot;

        uint64_t allocated_curr;

        uint64_t allocation_count;
        uint64_t free_count;

        public:
        StatMemoryMeter()
        :   allocated_max(0), allocated_largest(0), allocated_smallest(0), allocated_avg(0), allocated_tot(0),
            freed_tot(0), allocation_count(0), free_count(0) {}

        void record_alloc(uint64_t size);
        void record_free(uint64_t size);
    
        void reset();
        void report(std::string& meter_name) const;

        template<typename T_RET, typename T_ARG> void attach(T_RET(*caller)(T_ARG), T_ARG caller_arg);
        template<typename T_RET, typename T_ARG> void attach(T_RET(*caller)(T_ARG), T_ARG caller_arg, void(*then)(T_RET, StatMemoryMeter&));
    };

    class StatUCounterMeter {
        private:
        uint64_t count;

        public:
        void add(uint64_t val);
        void sub(uint64_t val);
        void set(uint64_t val);

        uint64_t value();

        void reset();
        void report(std::string& meter_name) const;

        template<typename T_RET, typename T_ARG> void attach(T_RET(*caller)(T_ARG), T_ARG caller_arg);
        template<typename T_RET, typename T_ARG> void attach(T_RET(*caller)(T_ARG), T_ARG caller_arg, void(*then)(T_RET, StatUCounterMeter&));
    };
};

#endif
