#include <u.h>
#include <libc.h>

void
main(int argc, char **argv)
{
	USED(argc);
	USED(argv);
	_exits("crash!");
}

