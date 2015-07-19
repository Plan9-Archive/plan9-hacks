#include <u.h>
#include <libc.h>

/*

8c curalloc.c
8l curalloc.8
p=`{8.out >[2=1] | awk '{ print $2 }' | tr -d : }
echo '*mainmem' | acid -lpool $p

-> curalloc	4294967016

*/

void
domalloc(int n)
{
	int i;
	void **a;

	a = mallocz(n * sizeof(void*), 1);

	for(i = 0; i < n; i++){
		a[i] = malloc(1024*1024*5);
	}

	for(i = 0; i < n; i++){
		free(a[i]);
	}

	free(a);
}

void
main(int argc, char *argv[])
{
	ARGBEGIN{
	}ARGEND

	domalloc(2);
	abort();
}

