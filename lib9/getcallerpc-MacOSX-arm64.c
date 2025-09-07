#include <lib9.h>

uintptr
getcallerpc(void *x)
{
	return (uintptr) __builtin_return_address(0); // modern compilers rule
}