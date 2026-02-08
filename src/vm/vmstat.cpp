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
}