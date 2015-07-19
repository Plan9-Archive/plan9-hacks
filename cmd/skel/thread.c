#include <u.h>
#include <libc.h>
#include <thread.h>

void
threadmain(int argc, char **argv)
{
	ARGBEGIN{
	}ARGEND

	print("Hello, world!\n");

	threadexits(nil);
}
