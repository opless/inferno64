/* was
	.file	"getcallerpc-MacOSX-amd64.s"
	.text
.globl _getcallerpc
_getcallerpc:
	pushq   %rbp
        movq    %rsp, %rbp
        movl    %edi, -4(%rbp)
        movq    8(%rbp), %rax
        popq    %rbp
        retq
*/

/* in drawterm
uintptr
getcallerpc(void *a)
{
	return ((uintptr*)a)[-1];
}
*/

#include <lib9.h>

uintptr
getcallerpc(void *x)
{
	//uintptr *lp;
	//	lp = x;
	//	return lp[-1];
	return (uintptr) __builtin_return_address(0); // modern compilers rule
}