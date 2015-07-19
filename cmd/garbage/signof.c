#include <u.h>
#include <libc.h>

void
main(void)
{
	print("%ux\n", signof(main));
	exits(nil);
}

