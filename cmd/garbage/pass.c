#include <u.h>
#include <libc.h>
#include <authsrv.h>

void
main(int argc, char **argv)
{
	int i;
	char key[DESKEYLEN];

	USED(argc);
	USED(argv);

	passtokey(key, "noisebridge");
	for(i = 0; i < DESKEYLEN; i++){
		print("%hhux", key[i]);
	}
	print("\n");
}

