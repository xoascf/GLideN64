#ifndef N64_H
#define N64_H

#include "Types.h"

#define TMEM_SIZE (1024 * 1024 * 4)
#define LOAD_BLOCK32_MAX (0x0400 * 16)
#define LOAD_BLOCK32_MASK ((0x0400 * 16) - 1)

#define MI_INTR_DP		0x20		// Bit 5: DP intr

struct N64Regs
{
#ifdef NATIVE
    N64Regs();
    ~N64Regs();
#endif
	u32 *MI_INTR;

	word *DPC_START;
	word *DPC_END;
	word *DPC_CURRENT;
	word *DPC_STATUS;
	u32 *DPC_CLOCK;
	u32 *DPC_BUFBUSY;
	u32 *DPC_PIPEBUSY;
	u32 *DPC_TMEM;

	u32 *VI_STATUS;
	u32 *VI_ORIGIN;
	u32 *VI_WIDTH;
	u32 *VI_INTR;
	u32 *VI_V_CURRENT_LINE;
	u32 *VI_TIMING;
	u32 *VI_V_SYNC;
	u32 *VI_H_SYNC;
	u32 *VI_LEAP;
	u32 *VI_H_START;
	u32 *VI_V_START;
	u32 *VI_V_BURST;
	u32 *VI_X_SCALE;
	u32 *VI_Y_SCALE;

	u32 *SP_STATUS;
};

extern N64Regs REG;
extern u8 *HEADER;

#if defined(NATIVE) && !defined(__clang__)
#define RDRAM ((u8*)0)
#define DMEM ((u8*)0)
#define IMEM ((u8*)0)
#else
extern u8* DMEM;
extern u8* IMEM;
extern u8* RDRAM;
#endif

extern u64 TMEM[TMEM_SIZE];
extern word RDRAMSize;
extern bool ConfigOpen;

#endif

