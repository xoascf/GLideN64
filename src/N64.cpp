#include "N64.h"

#ifndef NATIVE
u8* HEADER;
u8 *DMEM;
u8 *IMEM;
#endif
u64 TMEM[TMEM_SIZE];
#ifndef NATIVE
u8 *RDRAM;
#endif

#if defined(NATIVE) && defined(__clang__)
u8* DMEM = 0;
u8* IMEM = 0;
u8* RDRAM = 0;
#endif

word RDRAMSize = 0;

N64Regs REG;

bool ConfigOpen = false;

