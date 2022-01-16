#include "RSP.h"
#include "3DMath.h"

#define ENDIAN_BIT_SWAP 0

void RSP_LoadMatrix( f32 mtx[4][4], word address )
{
    struct _N64Matrix
    {
        s16 integer[4][4];
        u16 fraction[4][4];
    } *n64Mat = (struct _N64Matrix *)&RDRAM[address];

#ifdef NATIVE2
    for (u32 i = 0; i < 4; i++)
        for (u32 j = 0; j < 4; j++)
            mtx[i][j] = GetFloatMatrixElement(n64Mat->integer[i][j], n64Mat->fraction[i][j]);
#else
    for (u32 i = 0; i < 4; i++)
        for (u32 j = 0; j < 4; j++)
            mtx[i][j] = GetFloatMatrixElement(n64Mat->integer[i][j ^ ENDIAN_BIT_SWAP], n64Mat->fraction[i][j ^ ENDIAN_BIT_SWAP]);
#endif
}
