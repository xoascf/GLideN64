#ifndef RDP_H
#define RDP_H

#define MAXCMD 0x100000
const unsigned int maxCMDMask = MAXCMD - 1;

typedef struct
{
	word w0, w1, w2, w3;
	word cmd_ptr;
	word cmd_cur;
	word cmd_data[MAXCMD + 32];
} RDPInfo;

extern RDPInfo RDP;

void RDP_Init();
void RDP_Half_1(u32 _c);
void RDP_TexRect(const Gwords words);
void RDP_ProcessRDPList();
void RDP_RepeatLastLoadBlock();
void RDP_SetScissor(const Gwords words);
void RDP_SetTImg(const Gwords words);
void RDP_LoadBlock(const Gwords words);
void RDP_SetTile(const Gwords words);
void RDP_SetTileSize(const Gwords words);

#endif

