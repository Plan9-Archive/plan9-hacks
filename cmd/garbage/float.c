#include <u.h>
#include <libc.h>

int i = 0x80000000;

void
main(void)
{
	print("%f\n", i);
	exits(nil);
}
