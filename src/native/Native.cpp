#include "PluginAPI.h"
#include "N64.h"
#include "RSP.h"
#include "Native.h"
#include "GLideN64.h"
#include "Config.h"
#include "DebugDump.h"
#include "Config.h"
#include "DisplayWindow.h"
#include "FrameBuffer.h"
#include "FrameBufferInfo.h"
#include <wchar.h>
#include "settings.h"

#define START_WIDTH 1280
#define START_HEIGHT 720

static u64 g_width  = START_WIDTH;
static u64 g_height = START_HEIGHT;

extern "C" {
    u64 gfx_width()
    {
        return g_width;
    }

    u64 gfx_height()
    {
        return g_height;
    }
}

#ifdef WTL_UI
void ConfigInit(void* hinst);
void ConfigCleanup(void);
#endif

N64Regs::N64Regs() {
MI_INTR = new u32;
DPC_START = new word;
DPC_END = new word;
DPC_CURRENT = new word;
DPC_STATUS = new word;
DPC_CLOCK = new u32;
DPC_BUFBUSY = new u32;
DPC_PIPEBUSY = new u32;
DPC_TMEM = new u32;
VI_STATUS = new u32;
VI_ORIGIN = new u32;
VI_WIDTH = new u32;
VI_INTR = new u32;
VI_V_CURRENT_LINE = new u32;
VI_TIMING = new u32;
VI_V_SYNC = new u32;
VI_H_SYNC = new u32;
VI_LEAP = new u32;
VI_H_START = new u32;
VI_V_START = new u32;
VI_V_BURST = new u32;
VI_X_SCALE = new u32;
VI_Y_SCALE = new u32;
SP_STATUS = new u32;

*MI_INTR = 0;
*DPC_START = 0;
*DPC_END = 0;
*DPC_CURRENT = 0;
*DPC_STATUS = 0;
*DPC_CLOCK = 0;
*DPC_BUFBUSY = 0;
*DPC_PIPEBUSY = 0;
*DPC_TMEM = 0;
*VI_STATUS = 0;
*VI_ORIGIN = 0;
*VI_WIDTH = 320;
*VI_INTR = 0;
*VI_V_CURRENT_LINE = 0;
*VI_TIMING = 0;
*VI_V_SYNC = 0;
*VI_H_SYNC = 0;
*VI_LEAP = 0;
*VI_H_START = 0;
*VI_V_START = 0;
*VI_V_BURST = 0;
*VI_X_SCALE = 1024;
*VI_Y_SCALE = 512;
*SP_STATUS = 0;
}

N64Regs::~N64Regs() {
}

extern "C"
{
    void gfx_resize(long width, long height)
    {
        g_width = width;
        g_height = height;
        config.video.windowedWidth  = g_width;
        config.video.windowedHeight = g_height;
        dwnd().setWindowSize(g_width, g_height);
    }
}

void _CheckInterrupts() {
}



//Copied over from OOT - needs cleanup
struct Gfx;
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
//End of copy


extern "C" {
    void gfx_init(const char* romName, OSViMode* viMode) {
        REG.VI_STATUS  = &viMode->comRegs.ctrl;
        REG.VI_WIDTH   = &viMode->comRegs.width;
        REG.VI_TIMING  = &viMode->comRegs.burst;
        REG.VI_V_SYNC  = &viMode->comRegs.vSync;
        REG.VI_H_SYNC  = &viMode->comRegs.hSync;
        REG.VI_LEAP    = &viMode->comRegs.leap;
        REG.VI_H_START = &viMode->comRegs.hStart;
        REG.VI_X_SCALE = &viMode->comRegs.xScale;
        REG.VI_V_CURRENT_LINE = &viMode->comRegs.vCurrent;

        REG.VI_ORIGIN  = &viMode->fldRegs->origin;
        REG.VI_Y_SCALE = &viMode->fldRegs->yScale;
        REG.VI_V_START = &viMode->fldRegs->vStart;
        REG.VI_V_BURST = &viMode->fldRegs->vBurst;
        REG.VI_INTR    = &viMode->fldRegs->vIntr;

        CheckInterrupts = _CheckInterrupts;

        //StartDump(DEBUG_LOW | DEBUG_NORMAL | DEBUG_DETAIL | DEBUG_IGNORED | DEBUG_ERROR);

        REG.VI_STATUS;
        RDRAMSize = (word)-1;

        api().RomOpen(romName);
        //config.frameBufferEmulation.aspect = Config::aAdjust;
    }

    void gfx_shutdown() {
        RDRAMSize = 0;
        api().RomClosed();
    }

    void gfx_run(OSTask_t* task, u32 sz) {
        if(sizeof(OSTask_t) != sz)
        {
            return;
        }
        RSP_ProcessDList(task->data_ptr, task->data_size, task->ucode_boot, task->ucode_data, task->ucode_size);
        api().UpdateScreen();
        //Sleep(30);
    }

    int gfx_fbe_is_enabled() {
        return config.frameBufferEmulation.enable;
    }

    void gfx_fbe_enable(int enable) {
        config.frameBufferEmulation.enable = enable;
        //gfx_resize(g_width, g_height);
    }

    void gfx_fbe_sync(GraphicsContext* gfxCtx, GameInfo* GameInfo) {
        if (!config.frameBufferEmulation.enable)
            return;

        CfbInfo* cfb = gfxCtx->task.framebuffer;//Current frame buffer (according the the game)
        FrameBufferList& frameBuffers = FrameBufferList::get();//GLideN64's frame buffer list

        if (!frameBuffers.getCurrent())
            return;

        gfxCtx->curFrameBuffer = &frameBuffers.getCurrent()->m_startAddress;
        gfxCtx->viMode->fldRegs->origin = frameBuffers.getCurrent()->m_startAddress;

        cfb->fb1 = gfxCtx->curFrameBuffer;
        cfb->swapBuffer = gfxCtx->curFrameBuffer;

        cfb->viMode   = gfxCtx->viMode;
        cfb->features = gfxCtx->viFeatures;
        cfb->xScale = gfxCtx->xScale;
        cfb->xScale = gfxCtx->yScale;
        cfb->unk_10 = 0;
        cfb->updateRate = R_UPDATE_RATE;


        REG.VI_STATUS  = &cfb->viMode->comRegs.ctrl;
        REG.VI_WIDTH   = &cfb->viMode->comRegs.width;
        REG.VI_TIMING  = &cfb->viMode->comRegs.burst;
        REG.VI_V_SYNC  = &cfb->viMode->comRegs.vSync;
        REG.VI_H_SYNC  = &cfb->viMode->comRegs.hSync;
        REG.VI_LEAP    = &cfb->viMode->comRegs.leap;
        REG.VI_H_START = &cfb->viMode->comRegs.hStart;
        REG.VI_X_SCALE = &cfb->viMode->comRegs.xScale;
        REG.VI_V_CURRENT_LINE = &cfb->viMode->comRegs.vCurrent;

        REG.VI_ORIGIN  = &cfb->viMode->fldRegs->origin;//This is incorrect REG.VI_ORIGIN should contain only 24 bits of the frame buffer address
        REG.VI_Y_SCALE = &cfb->viMode->fldRegs->yScale;
        REG.VI_V_START = &cfb->viMode->fldRegs->vStart;
        REG.VI_V_BURST = &cfb->viMode->fldRegs->vBurst;
        REG.VI_INTR    = &cfb->viMode->fldRegs->vIntr;
    }
}

Config config;

void Config_DoConfig(/*HWND hParent*/)
{
    /*if(ConfigOpen)
        return;

    wchar_t strIniFolderPath[PLUGIN_PATH_SIZE];
    api().FindPluginPath(strIniFolderPath);

    ConfigOpen = true;
    const u32 maxMsaa = dwnd().maxMSAALevel();
    const u32 maxAnisotropy = dwnd().maxAnisotropy();
    const bool bRestart = RunConfig(strIniFolderPath, api().isRomOpen() ? RSP.romname : nullptr, maxMsaa, maxAnisotropy);
    if(config.generalEmulation.enableCustomSettings != 0)
        LoadCustomRomSettings(strIniFolderPath, RSP.romname);
    config.validate();
    if(bRestart)
        dwnd().restart();
    ConfigOpen = false;*/
}

void LoadConfig(const wchar_t* _strFileName)
{
    std::string IniFolder;
    uint32_t slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, NULL, 0, NULL, NULL);
    IniFolder.resize(slength);
    slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, (LPSTR)IniFolder.c_str(), slength, NULL, NULL);
    IniFolder.resize(slength - 1); //Remove null end char

    loadSettings(IniFolder.c_str());
}

void LoadCustomRomSettings(const wchar_t* _strFileName, const char* _romName)
{
    std::string IniFolder;
    uint32_t slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, NULL, 0, NULL, NULL);
    IniFolder.resize(slength);
    slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, (LPSTR)IniFolder.c_str(), slength, NULL, NULL);
    IniFolder.resize(slength - 1); //Remove null end char

    loadCustomRomSettings(IniFolder.c_str(), _romName);
}

void Config_LoadConfig()
{
    wchar_t strIniFolderPath[PLUGIN_PATH_SIZE];
    api().FindPluginPath(strIniFolderPath);
    LoadConfig(strIniFolderPath);
    if(config.generalEmulation.enableCustomSettings != 0)
        LoadCustomRomSettings(strIniFolderPath, RSP.romname);
    config.validate();
}