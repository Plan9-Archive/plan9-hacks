#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <cursor.h>
#include "glen.h"

Mousectl *mc;
Keyboardctl *kc;
Screen *scr;
Win *actw, *cmdw;

static void
initwin(void)
{
	Rectangle r;

	scr = allocscreen(screen, display->black, 0);
	if(scr == nil)
		sysfatal("allocscreen: %r");
	
	r = screen->r;
	r.max.y = r.min.y + Dy(r) / 5;
	//cmdw = newwin(CMD, r, nil);
}

static void
loop(void)
{
	Rune r;

	Alt a[] = {
		{mc->c, &mc->Mouse, CHANRCV},
		{kc->c, &r, CHANRCV},
		{mc->resizec, nil, CHANRCV},
		{nil, nil, CHANEND},
	};

	for(;;){
		flushimage(display, 1);
		switch(alt(a)){
		case 0:
			if((mc->buttons & 1) != 0)
				/*winclick(mc)*/;
			if((mc->buttons & 4) != 0)
				return;
			break;
		case 1:
			if(actw != nil /*&& actw->tab->key != nil*/)
				/*actw->tab->key(actw, r);*/;
			break;
		case 2:
			//resize();
			break;
		}
	}
}

void
threadmain(int argc, char *argv[])
{
	ARGBEGIN{
	}ARGEND

	if(initdraw(nil, nil, "glen") < 0)
		sysfatal("initdraw: %r");
	initwin();
	mc = initmouse(nil, screen);
	if(mc == nil)
		sysfatal("initmouse: %r");
	kc = initkeyboard(nil);
	if(kc == nil)
		sysfatal("initkeyboard: %r");
	loop();
	threadexitsall(nil);
}

