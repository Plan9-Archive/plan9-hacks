#include <u.h>
#include <libc.h>

typedef struct foo foo;
struct foo
{
	int a[2];
};

void
main(void)
{
	foo f = (foo){ { 1, 1}, };
	exits(nil);
}

