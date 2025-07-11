#ifndef S98TYPES_H__
#define S98TYPES_H__

#define __inline	__inline__
#ifndef NEVER_REACH
#define NEVER_REACH
#endif

typedef int				Int;
typedef unsigned int	Uint;
typedef signed int		Int32;
typedef unsigned int	Uint32;
typedef signed short	Int16;
typedef unsigned short	Uint16;
typedef signed char		Int8;
typedef unsigned char	Uint8;
typedef char			Char;

typedef Int32 Sample;

#include <stdlib.h>
#include <memory.h>

#define XSLEEP(t)	_sleep(t)
#define XMALLOC(s)	malloc(s)
#define XREALLOC(p,s)	realloc(p,s)
#define XFREE(p)	free(p)
#define XMEMCPY(d,s,n)	memcpy(d,s,n)
#define XMEMSET(d,c,n)	memset(d,c,n)

#endif /* S98TYPES_H__ */
