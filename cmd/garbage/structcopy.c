#include <u.h>
#include <libc.h>
#include <draw.h>
#include <bench.h>

typedef struct kopimi kopimi;
struct kopimi
{
	int stuff[1000];
};

kopimi one, two;

void
init(void)
{
	int i;
	for(i = 0; i < nelem(one.stuff); i++){
		one.stuff[i] = 1;
		two.stuff[i] = 2;
	}
}

kopimi
kopival(kopimi a, kopimi b)
{
	kopimi r;
	int i;
	for(i = 0; i < nelem(r.stuff); i++)
		r.stuff[i] = a.stuff[i] + b.stuff[i];
	return r;
}

kopimi
kopiptr(kopimi *a, kopimi *b)
{
	kopimi r;
	int i;
	for(i = 0; i < nelem(r.stuff); i++)
		r.stuff[i] = a->stuff[i] + b->stuff[i];
	return r;
}

void
benchval(B *b)
{
	int i;
	kopimi r;
	for(i = 0; i < b->N; i++){
		r = kopival(one, two);
	}
}

void
benchptr(B *b)
{
	int i;
	kopimi r;
	for(i = 0; i < b->N; i++){
		r = kopiptr(&one, &two);
	}
}

void
main(void)
{
	benchinit();
	init();
	bench("val", benchval);
	bench("ptr", benchptr);
}
