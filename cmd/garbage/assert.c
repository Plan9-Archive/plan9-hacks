#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	ARGBEGIN{
	}ARGEND

	if(argc > 2)
		assert(argc < 2);

	exits(nil);
}

