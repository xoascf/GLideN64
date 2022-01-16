#ifndef ZSORT_H
#define ZSORT_H

struct zSortVDest{
	s16 sy;
	s16 sx;
	s32 invw;
	s16 yi;
	s16 xi;
	s16 wi;
	u8 fog;
	u8 cc;
};

typedef f32 M44[4][4];

void ZSort_Init();
void ZSort_RDPCMD( const Gwords words );
int Calc_invw( int _w );

#endif // ZSORT_H
