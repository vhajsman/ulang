#include "compiler.hpp"
#include <iomanip>
#include <ios>
#include <iostream>
#include <istream>
#include <string>

namespace ULang {
    void CompilerInstance::verbose_nl(std::string str, bool ignore_depth) {
        this->verbose_print("\n");
        if(this->verbose_depthLvl > 0) {
            if(ignore_depth) {
                this->verbose_print("\t");
            } else {
                for(int i = 0; i < this->verbose_depthLvl; i++)
                    this->verbose_print("  ");
                this->verbose_print(" --> ");
            }
        }

        this->verbose_print(str);
    }

    void CompilerInstance::verbose_print(std::string str) {
        if(!this->cparams.verbose) return;
        std::cout << str;
    }

    void CompilerInstance::verbose_print(int val, int base, int pad_width) {
        if(!this->cparams.verbose) return;

        if(base == 16) std::cout << "0x";
        std::cout << std::setw(pad_width) << std::setfill('0');
        if(base == 16) std::cout << std::hex;
        std::cout << val;
    }

    /*
    inline void CompilerInstance::verbose_ascend() {
        this->verbose_depthLvl++;
    }

    inline void CompilerInstance::verbose_descend() {
        if(this->verbose_depthLvl > 0) this->verbose_depthLvl--;
    }
    */
};
