#include <cstdint>

namespace ULang {
    struct VMReg_define {
        uint32_t reg_no;
        const char* reg_name;
    };

    constexpr VMReg_define vmreg_defines[] = {
    // --- general purpose registers ---
    {0x00, "GPR0A"},
    {0x01, "GPR0B"},
    {0x02, "GPR0C"},
    {0x03, "GPR0D"},
    {0x04, "GPR1A"},
    {0x05, "GPR1B"},
    {0x06, "GPR1C"},
    {0x07, "GPR1D"},

    // --- modular interface registers ---
    {0x08, "MIA"},
    {0x09, "MIR"},
    {0x0a, "MID0"},
    {0x0b, "MID1"},

    // --- special purpose / control registers ---
    {0x0c, "SP"},
    {0x0d, "FP"},
    {0x0e, "PC"},
    {0x0f, "FLAGS"},

    // --- temporary / scratch registers ---
    {0x10, "TMP0"},
    {0x11, "TMP1"},
    {0x12, "TMP2"},
    {0x13, "TMP3"},


    {0xff, "EXC"},
};
};