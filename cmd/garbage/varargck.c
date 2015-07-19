#include <u.h>
#include <libc.h>

uchar data[] = { 0x1, 0x2, 0x3, 0x4, 0x5 };

int pprint(char*, ...);
#pragma varargck argpos pprint	1

void
main(void)
{
	void *p;
	fmtinstall('H', encodefmt);

	p = data;
	pprint("%s: %.*H\n", "data", sizeof data, p);
}

int
pprint(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	vfprint(2, fmt, arg);
	va_end(arg);

	return 0;
}
