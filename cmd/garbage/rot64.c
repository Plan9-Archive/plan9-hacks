#include <u.h>
#include <libc.h>

#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))

void
main(int argc, char **argv)
{
	uvlong a = 0x1020304050607080ULL;
	uvlong rot = 23;
	print("%llux\n", ROTL64(a, rot));
}
