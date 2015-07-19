#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>

Frame		f;
Image		**cols;

Mousectl	*msctl;
Keyboardctl	*kbctl;

enum {
	KBD, MOUSE, RESIZE, NALT
};

Channel *msc;
Channel *kbc;

void
mkcols(void)
{
	cols = mallocz(NCOL * sizeof(Image *), 1);
	if(cols == nil)
		sysfatal("mallocz: %r");

	cols[BACK] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x33333300);
	cols[HIGH] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x00990000);
	cols[BORD] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xAAAAAA00);
	cols[TEXT] = display->white;
	cols[HTEXT] = display->white;
}

void
redraw(int new)
{
	if(new && getwindow(display, Refmesg) < 0)
		sysfatal("can't reattach to window");

	draw(screen, screen->r, cols[BACK], nil, ZP);
	flushimage(display, 1);
}

void
inproc(void *)
{
	Mouse m;
	Rune r;
	Alt alts[NALT+1];

	alts[KBD].c = kbctl->c;
	alts[KBD].v = &r;
	alts[KBD].op = CHANRCV;
	alts[MOUSE].c = msctl->c;
	alts[MOUSE].v = &m;
	alts[MOUSE].op = CHANRCV;
	alts[RESIZE].op = CHANNOP;
	alts[NALT].op = CHANEND;

	for(;;) {
		switch(alt(alts)) {
		case MOUSE:
			send(msc, &m);
			break;
		case KBD:
			send(kbc, &r);
			break;
		}
	}
}

enum {
	MAXTEXT = 0x4000,
};

Rune *text;
ulong ntext;

void
threadmain(int, char *[])
{
	text = mallocz(sizeof(Rune) * MAXTEXT, 1);
	ulong ntext = 0;

	if(initdraw(nil, nil, argv0) < 0)
		sysfatal("initdraw: %r");

	if((msctl = initmouse(nil, screen)) == nil)
		sysfatal("initmouse: %r");

	if((kbctl = initkeyboard(nil)) == nil)
		sysfatal("initkeyboard: %r");

	mkcols();

	msc = chancreate(sizeof(Mouse), 0);
	kbc = chancreate(sizeof(Rune), 0);
	threadcreate(inproc, nil, 1024);

	draw(screen, screen->r, cols[BACK], nil, ZP);

	frinit(&f, screen->r, font, screen, cols);

	Rune r[2];
	Mouse m, om;
	Alt alts[NALT+1];

	r[1] = 0;

	alts[KBD].op = CHANRCV;
	alts[KBD].c = kbc;
	alts[KBD].v = &r[0];
	alts[MOUSE].op = CHANRCV;
	alts[MOUSE].c = msc;
	alts[MOUSE].v = &m;
	alts[RESIZE].op = CHANRCV;
	alts[RESIZE].c = msctl->resizec;
	alts[RESIZE].v = nil;
	alts[NALT].op = CHANEND;

	ulong start, end; // for deleting lines with ^u

	for(;;) {
		switch(alt(alts)) {
		case KBD:
			switch(r[0]) {
			case 0x1b:
			case 0x7F:
				threadexitsall(nil);
			case 0x15:
				// ctrl-u handling. very poor.
				if(ntext == 0)
					break;
				end = ntext;
				//fprint(2, "end %uld\n", end);
				for(start = ntext; start != 0; start--) {
					//fprint(2, "start %uld %x\n", start, text[start]);
					if(start == 0)
						break;
					if(text[start] == '\n') {
						//start++; // fragile, do not touch
						break;
					}
				}

				//fprint(2, "final start %uld end %uld\n", start, end);
				if((end - start > 0 || start == 0) && end > 0 ) {
					frdelete(&f, start, end);
					memset(&text[start], 0, sizeof(Rune) * (end - start));
					ntext = start;
				}
				//fprint(2, "remains: %S\n", text);
				break;
			case 0x08:
				if(ntext > 0) {
					frdelete(&f, ntext-1, ntext);
					text[ntext] = 0;
					ntext--;
				}
				break;
			default:
				if(ntext < MAXTEXT) {
					text[ntext++] = r[0];
					frinsert(&f, &r[0], &r[1], f.nchars);
				}
			}

			break;
		case MOUSE:
			if(m.buttons & 1 && m.buttons != om.buttons)
				frselect(&f, msctl);

			om = m;
			break;
		case RESIZE:
			redraw(1);
			frclear(&f, 1);
			frinit(&f, screen->r, font, screen, cols);
			frinsert(&f, text, text+ntext, 0);
			break;
		default:
			threadexitsall(nil);
		}

		flushimage(display, 1);
	}
}
