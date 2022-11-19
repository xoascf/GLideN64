#pragma once

typedef struct {
    /* 0x00 */ u32 ctrl;
    /* 0x04 */ u32 width;
    /* 0x08 */ u32 burst;
    /* 0x0C */ u32 vSync;
    /* 0x10 */ u32 hSync;
    /* 0x14 */ u32 leap;
    /* 0x18 */ u32 hStart;
    /* 0x1C */ u32 xScale;
    /* 0x20 */ u32 vCurrent;
} OSViCommonRegs; // size = 0x20

typedef struct {
    /* 0x00 */ u32 origin;
    /* 0x04 */ u32 yScale;
    /* 0x08 */ u32 vStart;
    /* 0x0C */ u32 vBurst;
    /* 0x10 */ u32 vIntr;
} OSViFieldRegs; // size = 0x14

typedef struct {
    /* 0x00 */ u8 type;
    /* 0x04 */ OSViCommonRegs comRegs;
    /* 0x24 */ OSViFieldRegs fldRegs[2];
} OSViMode; // size = 0x4C

typedef struct {
    /* 0x00 */ u32 type;
    /* 0x04 */ u32 flags;

    /* 0x08 */ u64* ucode_boot;
    /* 0x0C */ u32 ucode_boot_size;

    /* 0x10 */ u64* ucode;
    /* 0x14 */ u32 ucode_size;

    /* 0x18 */ u64* ucode_data;
    /* 0x1C */ u32 ucode_data_size;

    /* 0x20 */ u64* dram_stack;
    /* 0x24 */ u32 dram_stack_size;

    /* 0x28 */ u64* output_buff;
    /* 0x2C */ u64* output_buff_size;

    /* 0x30 */ u64* data_ptr;
    /* 0x34 */ u32 data_size;

    /* 0x38 */ u64* yield_data_ptr;
    /* 0x3C */ u32 yield_data_size;
} OSTask_t; // size = 0x40

//Copied over from OOT - needs cleanup
union Gfx;
struct OSThread;
typedef void* OSMesg;

typedef struct OSMesgQueue {
    /* 0x00 */ OSThread* mtqueue;
    /* 0x04 */ OSThread* fullqueue;
    /* 0x08 */ s32 validCount;
    /* 0x0C */ s32 first;
    /* 0x10 */ s32 msgCount;
    /* 0x14 */ OSMesg* msg;
} OSMesgQueue; // size = 0x18

typedef struct {
    /* 0x0000 */ u32 size;
    /* 0x0004 */ Gfx* bufp;
    /* 0x0008 */ Gfx* p;
    /* 0x000C */ Gfx* d;
} TwoHeadGfxArena; // size = 0x10

typedef struct CfbInfo {
    /* 0x00 */ u32* fb1;//Address to the frame buffer
    /* 0x04 */ u32* swapBuffer;
    /* 0x08 */ OSViMode* viMode;
    /* 0x0C */ u32 features;
    /* 0x10 */ u8 unk_10;
    /* 0x11 */ s8 updateRate;
    /* 0x12 */ s8 updateRate2;
    /* 0x13 */ u8 unk_13;
    /* 0x14 */ f32 xScale;
    /* 0x18 */ f32 yScale;
} CfbInfo; // size = 0x1C

typedef union {
    OSTask_t t;
    long long int force_structure_alignment;
} OSTask;

typedef struct OSScTask {
    /* 0x00 */ struct OSScTask* next;
    /* 0x04 */ u32 state;
    /* 0x08 */ u32 flags;
    /* 0x0C */ CfbInfo* framebuffer;
    /* 0x10 */ OSTask list;
    /* 0x50 */ OSMesgQueue* msgQ;
    /* 0x54 */ OSMesg msg;
} OSScTask;

typedef struct GraphicsContext {
    /* 0x0000 */ Gfx* polyOpaBuffer; // Pointer to "Zelda 0"
    /* 0x0004 */ Gfx* polyXluBuffer; // Pointer to "Zelda 1"
    /* 0x0008 */ char unk_008[0x08]; // Unused, could this be pointers to "Zelda 2" / "Zelda 3"
    /* 0x0010 */ Gfx* overlayBuffer; // Pointer to "Zelda 4"
    /* 0x0014 */ u32 unk_014;
    /* 0x0018 */ char unk_018[0x20];
    /* 0x0038 */ OSMesg msgBuff[0x08];
    /* 0x0058 */ OSMesgQueue* schedMsgQ;
    /* 0x005C */ OSMesgQueue queue;
    /* 0x0074 */ char unk_074[0x04];
    /* 0x0078 */ OSScTask task; // size of OSScTask might be wrong
    /* 0x00D0 */ char unk_0D0[0xE0];
    /* 0x01B0 */ Gfx* workBuffer;
    /* 0x01B4 */ TwoHeadGfxArena work;
    /* 0x01C4 */ char unk_01C4[0xC0];
    /* 0x0284 */ OSViMode* viMode;
    /* 0x0288 */ char unk_0288[0x20];     // Unused, could this be Zelda 2/3 ?
    /* 0x02A8 */ TwoHeadGfxArena overlay; // "Zelda 4"
    /* 0x02B8 */ TwoHeadGfxArena polyOpa; // "Zelda 0"
    /* 0x02C8 */ TwoHeadGfxArena polyXlu; // "Zelda 1"
    /* 0x02D8 */ u32 gfxPoolIdx;
    /* 0x02DC */ u32* curFrameBuffer;
    /* 0x02E0 */ char unk_2E0[0x04];
    /* 0x02E4 */ u32 viFeatures;
    /* 0x02E8 */ s32 fbIdx;
    /* 0x02EC */ void (*callback)(struct GraphicsContext*, void*);
    /* 0x02F0 */ void* callbackParam;
    /* 0x02F4 */ f32 xScale;
    /* 0x02F8 */ f32 yScale;
    /* 0x02FC */ char unk_2FC[0x04];
} GraphicsContext; // size = 0x300

#define REG_GROUPS 29 // number of REG groups, i.e. REG, SREG, OREG, etc.
#define REG_PAGES 6
#define REG_PER_PAGE 16
#define REG_PER_GROUP REG_PAGES * REG_PER_PAGE

typedef struct {
    /* 0x00 */ s32 regPage;  // 1 is first page
    /* 0x04 */ s32 regGroup; // "register" group (R, RS, RO, RP etc.)
    /* 0x08 */ s32 regCur;   // selected register within page
    /* 0x0C */ s32 dpadLast;
    /* 0x10 */ s32 repeat;
    /* 0x14 */ s16 data[REG_GROUPS * REG_PER_GROUP]; // 0xAE0 entries
} GameInfo;                                          // size = 0x15D4


#define BASE_REG(n, r) GameInfo->data[n * REG_PER_GROUP + r]

#define SREG(r) BASE_REG(1, r)

#define R_UPDATE_RATE               SREG(30)

extern "C" {
    void gfx_init(const char* romName, OSViMode* viMode);
    void gfx_shutdown();
    void gfx_start_frame();
    void gfx_end_frame();
    void gfx_run(OSTask_t* task, u32 sz);
}