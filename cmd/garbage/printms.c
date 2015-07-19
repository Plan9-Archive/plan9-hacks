#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

void
eresized(int new)
{
	if(getwindow(display, Refnone) < 0)
		sysfatal("resize failed: %r");
}

void
main(int argc, char **argv)
{
	Mouse m;

	USED(argc);
	USED(argv);

	initdraw(nil, nil, "printms");
	einit(Emouse);

	for(;;){
		while(ecanmouse()){
			m = emouse();
			print("%d %d	%08x %08x\n", m.xy.x, m.xy.y, m.xy.x, m.xy.y);
		}
	}
}
