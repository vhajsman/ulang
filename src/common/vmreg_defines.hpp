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

#define R_GPR0A vmreg_defines[0]
#define R_GPR0B vmreg_defines[1]
#define R_GPR0C vmreg_defines[2]
#define R_GPR0D vmreg_defines[3]
#define R_GPR1A vmreg_defines[4]
#define R_GPR1B vmreg_defines[5]
#define R_GPR1C vmreg_defines[6]
#define R_GPR1D vmreg_defines[7]
#define R_MIA   vmreg_defines[8]
#define R_MIR   vmreg_defines[9]
#define R_MID0  vmreg_defines[10]
#define R_MID1  vmreg_defines[11]
#define R_SP    vmreg_defines[12]
#define R_FP    vmreg_defines[13]
#define R_PC    vmreg_defines[14]
#define R_FLAGS vmreg_defines[15]
#define R_TMP0  vmreg_defines[16]
#define R_TMP1  vmreg_defines[17]
#define R_TMP2  vmreg_defines[18]
#define R_TMP3  vmreg_defines[19]
#define R_EXC   vmreg_defines[20]