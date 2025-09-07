#include "lib9.h"
#include <a.out.h>
#include <dynld.h>

#define	CHK(i,ntab)	if((unsigned)(i)>=(ntab))return "bad relocation index"

long
dynmagic(void)
{
	return DYN_MAGIC | I_MAGIC;
}

char*
dynreloc(uchar *b, uintptr p, int m, Dynsym **tab, int ntab)
{
	return "not implemented on arm64";
}
