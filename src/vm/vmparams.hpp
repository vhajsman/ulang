#ifndef __ULANG_VMPARAMS_H
#define __ULANG_VMPARAMS_H

#include <cstddef>
#include <string>

namespace ULang {
    struct VMParams {
        std::string fileName;
        
        bool verbose_en;
        bool stats_en;

        size_t heapsize_start_kb;
        size_t heapsize_limit_kb;
    };
};

#endif
