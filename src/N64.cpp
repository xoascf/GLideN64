#include "N64.h"

#ifndef NATIVE
u8* HEADER;
u8 *DMEM;
u8 *IMEM;
#endif
u64 TMEM[512];
#ifndef NATIVE
u8 *RDRAM;
#endif

word RDRAMSize = 0;

N64Regs REG;

bool ConfigOpen = false;
