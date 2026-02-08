#ifndef __ULANG_VMSTAT_H
#define __ULANG_VMSTAT_H

#include <chrono>
#include <string>
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
};

#endif
