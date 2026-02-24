#include "errno.h"
#include <string>

namespace ULang {
    struct SourceLocation {
        void* loc_parent;
        std::string loc_file;
        size_t loc_line;
        size_t loc_col;
    };
};