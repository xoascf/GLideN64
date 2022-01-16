#ifndef S2DEX_H
#define S2DEX_H

#include "Types.h"

void S2DEX_BG_1Cyc(const Gwords words);
void S2DEX_BG_Copy( const Gwords words );
void S2DEX_Obj_Rectangle( const Gwords words );
void S2DEX_Obj_Sprite( const Gwords words );
void S2DEX_Obj_MoveMem( const Gwords words );
void S2DEX_RDPHalf_0( const Gwords words );
void S2DEX_Select_DL( const Gwords words );
void S2DEX_Obj_RenderMode( const Gwords words );
void S2DEX_Obj_Rectangle_R( const Gwords words );
void S2DEX_Obj_LoadTxtr( const Gwords words );
void S2DEX_Obj_LdTx_Sprite( const Gwords words );
void S2DEX_Obj_LdTx_Rect( const Gwords words );
void S2DEX_Obj_LdTx_Rect_R( const Gwords words );
void S2DEX_1_03_Init();
void S2DEX_1_05_Init();
void S2DEX_1_07_Init();
void resetObjMtx();

#endif
