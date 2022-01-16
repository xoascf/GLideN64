#include "GLideN64.h"
#include "DebugDump.h"
#include "F3D.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"

void F3D_SPNoOp( const Gwords words )
{
	gSPNoOp();
}

void F3D_Mtx( const Gwords words )
{
	if (_SHIFTR( words.w0, 0, 16 ) != 64) {
		DebugMsg(DEBUG_NORMAL | DEBUG_ERROR, "G_MTX: address = 0x%08X    length = %i    params = 0x%02X\n", words.w1, _SHIFTR(words.w0, 0, 16), _SHIFTR(words.w0, 16, 8));
		return;
	}

	gSPMatrix( words.w1, _SHIFTR( words.w0, 16, 8 ) );
}

void F3D_Reserved0( const Gwords words )
{
	DebugMsg(DEBUG_NORMAL | DEBUG_IGNORED, "G_RESERVED0: words.w0=0x%08lX words.w1=0x%08lX\n", words.w0, words.w1);
}

void F3D_MoveMem( const Gwords words )
{
	switch (_SHIFTR( words.w0, 16, 8 )) {
		case F3D_MV_VIEWPORT:
			gSPViewport( words.w1 );
			break;
		case G_MV_MATRIX_1:
			gSPForceMatrix( words.w1 );

			// force matrix takes four commands
			RSP.PC[RSP.PCi] += 24;
			break;
		case G_MV_L0:
			gSPLight( words.w1, LIGHT_1 );
			break;
		case G_MV_L1:
			gSPLight( words.w1, LIGHT_2 );
			break;
		case G_MV_L2:
			gSPLight( words.w1, LIGHT_3 );
			break;
		case G_MV_L3:
			gSPLight( words.w1, LIGHT_4 );
			break;
		case G_MV_L4:
			gSPLight( words.w1, LIGHT_5 );
			break;
		case G_MV_L5:
			gSPLight( words.w1, LIGHT_6 );
			break;
		case G_MV_L6:
			gSPLight( words.w1, LIGHT_7 );
			break;
		case G_MV_L7:
			gSPLight( words.w1, LIGHT_8 );
			break;
		case G_MV_LOOKATX:
			gSPLookAt(words.w1, 0);
			break;
		case G_MV_LOOKATY:
			gSPLookAt(words.w1, 1);
			break;
	}
}

void F3D_Vtx( const Gwords words )
{
	gSPVertex( words.w1, _SHIFTR( words.w0, 20, 4 ) + 1, _SHIFTR( words.w0, 16, 4 ) );
}

void F3D_Reserved1( const Gwords words )
{
}

void F3D_DList( const Gwords words )
{
	switch (_SHIFTR( words.w0, 16, 8 ))
	{
		case G_DL_PUSH:
			gSPDisplayList( words.w1 );
			break;
		case G_DL_NOPUSH:
			gSPBranchList( words.w1 );
			break;
	}
}

void F3D_Reserved2( const Gwords words )
{
}

void F3D_Reserved3( const Gwords words )
{
}

void F3D_Sprite2D_Base( const Gwords words )
{
	gSPSprite2DBase( words.w1 );
}

void F3D_Tri1( const Gwords words )
{
	gSP1Triangle( _SHIFTR( words.w1, 16, 8 ) / 10,
				  _SHIFTR( words.w1, 8, 8 ) / 10,
				  _SHIFTR( words.w1, 0, 8 ) / 10);
}

void F3D_CullDL( const Gwords words )
{
	gSPCullDisplayList( _SHIFTR( words.w0, 0, 24 ) / 40, (words.w1 / 40) - 1 );
}

void F3D_PopMtx( const Gwords words )
{
	gSPPopMatrix( words.w1 );
}

void F3D_MoveWord( const Gwords words )
{
	switch (_SHIFTR( words.w0, 0, 8 )) {
		case G_MW_MATRIX:
			gSPInsertMatrix( _SHIFTR( words.w0, 8, 16 ), words.w1 );
			break;
		case G_MW_NUMLIGHT:
			gSPNumLights( ((words.w1 - 0x80000000) >> 5) - 1 );
			break;
		case G_MW_CLIP:
			gSPClipRatio( words.w1 );
			break;
		case G_MW_SEGMENT:
			gSPSegment( _SHIFTR( words.w0, 10, 4 ), SEGMENT_MASK(words.w1) );
			break;
		case G_MW_FOG:
/*			u32 fm, fo, min, max;

			fm = _SHIFTR( words.w1, 16, 16 );
			fo = _SHIFTR( words.w1, 0, 16 );

			min = 500 - (fo * (128000 / fm)) / 256;
			max = (128000 / fm) + min;*/

			gSPFogFactor( (s16)_SHIFTR( words.w1, 16, 16 ), (s16)_SHIFTR( words.w1, 0, 16 ) );
			break;
		case G_MW_LIGHTCOL:
			switch (_SHIFTR( words.w0, 8, 16 ))
			{
				case F3D_MWO_aLIGHT_1:
					gSPLightColor( LIGHT_1, words.w1 );
					break;
				case F3D_MWO_aLIGHT_2:
					gSPLightColor( LIGHT_2, words.w1 );
					break;
				case F3D_MWO_aLIGHT_3:
					gSPLightColor( LIGHT_3, words.w1 );
					break;
				case F3D_MWO_aLIGHT_4:
					gSPLightColor( LIGHT_4, words.w1 );
					break;
				case F3D_MWO_aLIGHT_5:
					gSPLightColor( LIGHT_5, words.w1 );
					break;
				case F3D_MWO_aLIGHT_6:
					gSPLightColor( LIGHT_6, words.w1 );
					break;
				case F3D_MWO_aLIGHT_7:
					gSPLightColor( LIGHT_7, words.w1 );
					break;
				case F3D_MWO_aLIGHT_8:
					gSPLightColor( LIGHT_8, words.w1 );
					break;
			}
			break;
		case G_MW_POINTS:
			{
			  const u32 val = _SHIFTR(words.w0, 8, 16);
			  gSPModifyVertex(val / 40, val % 40, words.w1);
			}
			break;
		case G_MW_PERSPNORM:
			gSPPerspNormalize( words.w1 );
			break;
	}
}

void F3D_Texture( const Gwords words )
{
	gSPTexture( _FIXED2FLOAT( _SHIFTR( words.w1, 16, 16 ), 16 ),
				_FIXED2FLOAT( _SHIFTR( words.w1, 0, 16 ), 16 ),
				_SHIFTR( words.w0, 11, 3 ),
				_SHIFTR( words.w0, 8, 3 ),
				_SHIFTR( words.w0, 0, 8 ) );
}

void F3D_SetOtherMode_H( const Gwords words )
{
	const u32 length = _SHIFTR(words.w0, 0, 8);
	const u32 shift = _SHIFTR(words.w0, 8, 8);
	gSPSetOtherMode_H(length, shift, words.w1);
}

void F3D_SetOtherMode_L( const Gwords words )
{
	const u32 length = _SHIFTR(words.w0, 0, 8);
	const u32 shift = _SHIFTR(words.w0, 8, 8);
	gSPSetOtherMode_L(length, shift, words.w1);
}

void F3D_EndDL( const Gwords words )
{
	gSPEndDisplayList(words);
}

void F3D_SetGeometryMode( const Gwords words )
{
	gSPSetGeometryMode( words.w1 );
}

void F3D_ClearGeometryMode( const Gwords words )
{
	gSPClearGeometryMode( words.w1 );
}

void F3D_Quad( const Gwords words )
{
	gSP1Quadrangle( _SHIFTR( words.w1, 24, 8 ) / 10, _SHIFTR( words.w1, 16, 8 ) / 10, _SHIFTR( words.w1, 8, 8 ) / 10, _SHIFTR( words.w1, 0, 8 ) / 10 );
}

void F3D_RDPHalf_1( const Gwords words )
{
	gDP.half_1 = words.w1;
	RDP_Half_1(words.w1);
}

void F3D_RDPHalf_2( const Gwords words )
{
	gDP.half_2 = words.w1;
}

void F3D_RDPHalf_Cont( const Gwords words )
{
}

void F3D_Init()
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
	GBI_SetGBI( G_VTX,					F3D_VTX,				F3D_Vtx );
	GBI_SetGBI( G_RESERVED1,			F3D_RESERVED1,			F3D_Reserved1 );
	GBI_SetGBI( G_DL,					F3D_DL,					F3D_DList );
	GBI_SetGBI( G_RESERVED2,			F3D_RESERVED2,			F3D_Reserved2 );
	GBI_SetGBI( G_RESERVED3,			F3D_RESERVED3,			F3D_Reserved3 );
	GBI_SetGBI( G_SPRITE2D_BASE,		F3D_SPRITE2D_BASE,		F3D_Sprite2D_Base );

	GBI_SetGBI( G_TRI1,					F3D_TRI1,				F3D_Tri1 );
	GBI_SetGBI( G_CULLDL,				F3D_CULLDL,				F3D_CullDL );
	GBI_SetGBI( G_POPMTX,				F3D_POPMTX,				F3D_PopMtx );
	GBI_SetGBI( G_MOVEWORD,				F3D_MOVEWORD,			F3D_MoveWord );
	GBI_SetGBI( G_TEXTURE,				F3D_TEXTURE,			F3D_Texture );
	GBI_SetGBI( G_SETOTHERMODE_H,		F3D_SETOTHERMODE_H,		F3D_SetOtherMode_H );
	GBI_SetGBI( G_SETOTHERMODE_L,		F3D_SETOTHERMODE_L,		F3D_SetOtherMode_L );
	GBI_SetGBI( G_ENDDL,				F3D_ENDDL,				F3D_EndDL );
	GBI_SetGBI( G_SETGEOMETRYMODE,		F3D_SETGEOMETRYMODE,	F3D_SetGeometryMode );
	GBI_SetGBI( G_CLEARGEOMETRYMODE,	F3D_CLEARGEOMETRYMODE,	F3D_ClearGeometryMode );
	GBI_SetGBI( G_QUAD,					F3D_QUAD,				F3D_Quad );
	GBI_SetGBI( G_RDPHALF_1,			F3D_RDPHALF_1,			F3D_RDPHalf_1 );
	GBI_SetGBI( G_RDPHALF_2,			F3D_RDPHALF_2,			F3D_RDPHalf_2 );
	GBI_SetGBI( G_RDPHALF_CONT,			F3D_RDPHALF_CONT,		F3D_RDPHalf_Cont );
}
