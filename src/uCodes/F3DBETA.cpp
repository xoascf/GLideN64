#include "GLideN64.h"
#include "DebugDump.h"
#include "F3D.h"
#include "F3DEX.h"
#include "F3DBETA.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"

void F3DBETA_Vtx( const Gwords words )
{
	gSPVertex( words.w1, _SHIFTR( words.w0, 9, 7 ), _SHIFTR( words.w0, 16, 8 ) / 5 );
}

void F3DBETA_Tri1( const Gwords words )
{
	gSP1Triangle( _SHIFTR( words.w1, 16, 8 ) / 5,
				  _SHIFTR( words.w1, 8, 8 ) / 5,
				  _SHIFTR( words.w1, 0, 8 ) / 5);
}

void F3DBETA_Tri2( const Gwords words )
{
	gSP2Triangles( _SHIFTR( words.w0, 16, 8 ) / 5, _SHIFTR( words.w0, 8, 8 ) / 5, _SHIFTR( words.w0, 0, 8 ) / 5, 0,
				   _SHIFTR( words.w1, 16, 8 ) / 5, _SHIFTR( words.w1, 8, 8 ) / 5, _SHIFTR( words.w1, 0, 8 ) / 5, 0);
}

void F3DBETA_Quad( const Gwords words )
{
	gSP1Quadrangle( _SHIFTR( words.w1, 24, 8 ) / 5, _SHIFTR( words.w1, 16, 8 ) / 5, _SHIFTR( words.w1, 8, 8 ) / 5, _SHIFTR( words.w1, 0, 8 ) / 5 );
}

void F3DBETA_Perpnorm(const Gwords words)
{
	gSPPerspNormalize(words.w1);
}

void F3DBETA_Init()
{
	gSPSetupFunctions();
	// Set GeometryMode flags
	GBI_InitFlags( F3D );

	GBI.PCStackSize = 10;

	//          GBI Command             Command Value			Command Function
	GBI_SetGBI( G_SPNOOP,				F3D_SPNOOP,				F3D_SPNoOp );
	GBI_SetGBI( G_MTX,					F3D_MTX,				F3D_Mtx );
	GBI_SetGBI( G_RESERVED0,			F3D_RESERVED0,			F3D_Reserved0 );
	GBI_SetGBI( G_MOVEMEM,				F3D_MOVEMEM,			F3D_MoveMem );
	GBI_SetGBI( G_VTX,					F3D_VTX,				F3DBETA_Vtx );
	GBI_SetGBI( G_RESERVED1,			F3D_RESERVED1,			F3D_Reserved1 );
	GBI_SetGBI( G_DL,					F3D_DL,					F3D_DList );
	GBI_SetGBI( G_RESERVED2,			F3D_RESERVED2,			F3D_Reserved2 );
	GBI_SetGBI( G_RESERVED3,			F3D_RESERVED3,			F3D_Reserved3 );
	GBI_SetGBI( G_SPRITE2D_BASE,		F3D_SPRITE2D_BASE,		F3D_Sprite2D_Base );

	GBI_SetGBI( G_TRI1,					F3D_TRI1,				F3DBETA_Tri1 );
	GBI_SetGBI( G_CULLDL,				F3D_CULLDL,				F3D_CullDL );
	GBI_SetGBI( G_POPMTX,				F3D_POPMTX,				F3D_PopMtx );
	GBI_SetGBI( G_MOVEWORD,				F3D_MOVEWORD,			F3D_MoveWord );
	GBI_SetGBI( G_TEXTURE,				F3D_TEXTURE,			F3D_Texture );
	GBI_SetGBI( G_SETOTHERMODE_H,		F3D_SETOTHERMODE_H,		F3D_SetOtherMode_H );
	GBI_SetGBI( G_SETOTHERMODE_L,		F3D_SETOTHERMODE_L,		F3D_SetOtherMode_L );
	GBI_SetGBI( G_ENDDL,				F3D_ENDDL,				F3D_EndDL );
	GBI_SetGBI( G_SETGEOMETRYMODE,		F3D_SETGEOMETRYMODE,	F3D_SetGeometryMode );
	GBI_SetGBI( G_CLEARGEOMETRYMODE,	F3D_CLEARGEOMETRYMODE,	F3D_ClearGeometryMode );
	GBI_SetGBI( G_QUAD,					F3D_QUAD,				F3DBETA_Quad );
	GBI_SetGBI( G_PERSPNORM,			F3DBETA_PERSPNORM,		F3DBETA_Perpnorm);
	GBI_SetGBI( G_RDPHALF_1,			F3DBETA_RDPHALF_1,		F3D_RDPHalf_1 );
	GBI_SetGBI( G_RDPHALF_2,			F3DBETA_RDPHALF_2,		F3D_RDPHalf_2 );
	GBI_SetGBI( G_TRI2,					F3DBETA_TRI2,			F3DBETA_Tri2 );
}

