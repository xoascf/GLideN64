#ifndef TYPES_H
#define TYPES_H

typedef unsigned char			u8;	/* unsigned  8-bit */
typedef unsigned short			u16;	/* unsigned 16-bit */
typedef unsigned int			u32;	/* unsigned 32-bit */
typedef unsigned long long		u64;	/* unsigned 64-bit */

typedef signed char			s8;	/* signed  8-bit */
typedef short				s16;	/* signed 16-bit */
typedef int				s32;	/* signed 32-bit */
typedef long long			s64;	/* signed 64-bit */

typedef volatile unsigned char		vu8;	/* unsigned  8-bit */
typedef volatile unsigned short		vu16;	/* unsigned 16-bit */
typedef volatile unsigned int		vu32;	/* unsigned 32-bit */
typedef volatile unsigned long long	vu64;	/* unsigned 64-bit */

typedef volatile signed char	vs8;	/* signed  8-bit */
typedef volatile short			vs16;	/* signed 16-bit */
typedef volatile int			vs32;	/* signed 32-bit */
typedef volatile long long		vs64;	/* signed 64-bit */

typedef float				f32;	/* single prec floating point */
typedef double				f64;	/* double prec floating point */

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef NULL
#define NULL    0
#endif

#ifndef PLUGIN_PATH_SIZE
#define PLUGIN_PATH_SIZE 260
#endif

template <typename T>
class ValueKeeper
{
public:
	ValueKeeper(T& _obj, T _newVal)
		: m_obj(_obj)
		, m_val(_obj)
	{
		m_obj = _newVal;
	}

	~ValueKeeper()
	{
		m_obj = m_val;
	}

private:
	T & m_obj;
	T m_val;
};

#include "porting.h"

struct Gwords {
    Gwords();
    Gwords(word _w0, word _w1);

    word w0;
    word w1;
#ifdef EXTENDED_GFX
    word w2;
    word w3;

	Gwords(word _w0, word _w1, word _w2, word _w3);
#endif
};

#ifdef EXTENDED_GFX
static_assert(sizeof(Gwords) == sizeof(word) * 4, "Gwords is incorrect size");
#else
static_assert(sizeof(Gwords) == sizeof(word) * 2, "Gwords is incorrect size");
#endif

struct RGBA8
{
#ifdef NATIVE
    u8 a;
    u8 b;
    u8 g;
    u8 r;
#else
    u8 r;
    u8 g;
    u8 b;
    u8 a;
#endif
};

static_assert(sizeof(RGBA8) == 4);

#endif // TYPES_H
