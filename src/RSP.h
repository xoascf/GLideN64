#ifndef RSP_H
#define RSP_H

#include "Types.h"
#include "N64.h"

#define MAX_SEGMENTS 16

typedef struct
{
	word PC[18], PCi;
	word F5DL[10];
	word uc_start, uc_dstart, cmd, nextCmd;
    Gwords words;
	s32 count;
	bool busy, halt, infloop;
	bool LLE;
#ifdef NATIVE
	bool translateSegment;
#endif
	char romname[21];
	wchar_t pluginpath[PLUGIN_PATH_SIZE];
} RSPInfo;

extern RSPInfo RSP;

extern u32 DepthClearColor;
extern u32 rectDepthBufferCopyFrame;

#ifdef NATIVE
//#define RSP_SegmentToPhysical( segaddr ) ((segaddr < (16 << 24)) ? ((gSP.segment[(segaddr >> 24) & 0x0F] + segaddr)) : segaddr)
//#define RSP_SegmentToPhysical( segaddr ) ((RSP_SegmentToPhysical2(segaddr) & 0x00FFFF00) != 0 ? segaddr : ((gSP.segment[(segaddr >> 24) & 0x0F] + segaddr)))
//#define RSP_SegmentToPhysical(segaddr) (segaddr)

word RSP_SegmentToPhysical(word segaddr);
#define SEGMENT_MASK(addr) (addr)

#else
#define SEGMENT_MASK(addr) (addr & 0x00FFFFFF)
#define RSP_SegmentToPhysical( segaddr ) ((gSP.segment[(segaddr >> 24) & 0x0F] + (segaddr & RDRAMSize)) & RDRAMSize)
#endif

void RSP_Init(const char* romName = nullptr);

void RSP_ProcessDList(void* displayList, word displayListLength, void* uc_start, void* uc_dstart, word uc_dsize, u32 matrixStackSize = 0, void* ZSortBOSS_pc = nullptr);
void RSP_ProcessDList();


void RSP_LoadMatrix( f32 mtx[4][4], word address );
void RSP_CheckDLCounter();

#endif
