#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include <memlayer.h>

static void
writeuncompressed(int fd, Memimage *m)
{
	char chanstr[32];
	int bpl, y, j;
	uchar *buf;

	if(chantostr(chanstr, m->chan) == nil)
		sysfatal("can't convert channel descriptor: %r");
	fprint(fd, "%11s %11d %11d %11d %11d ",
		chanstr, m->r.min.x, m->r.min.y, m->r.max.x, m->r.max.y);

	bpl = bytesperline(m->r, m->depth);
	buf = malloc(bpl);
	if(buf == nil)
		sysfatal("malloc: %r");
	for(y=m->r.min.y; y<m->r.max.y; y++){
		j = unloadmemimage(m, Rect(m->r.min.x, y, m->r.max.x, y+1), buf, bpl);
		if(j != bpl)
			sysfatal("image unload failed: %r");
		if(write(fd, buf, bpl) != bpl)
			sysfatal("write: %r");
	}
	free(buf);
}

static Point
memstr(Memimage *dst, Point pt, Memimage *src, Point sp, Font *f, Rune *r, int len, Drawop op)
{
	int i, n;
	ushort idx;
	Memsubfont *msf;
	Cachefont *cf;
	Fontchar *fc;

	if(r == nil || len == 0){
		return ZP;
	}

	msf = nil;
	cf = nil;
	while(*r != 0 && len > 0){
		/* open subfont */
		if(msf == nil){
			cf = nil;
			for(i = 0; i < f->nsub; i++){
				if(f->sub[i]->min <= *r && *r <= f->sub[i]->max){
					cf = f->sub[i];
					break;
				}
			}

			if(cf != nil){
				if(cf->subfontname == nil){
					cf->subfontname = subfontname(cf->name, f->name, 8);
				}
				msf = openmemsubfont(cf->subfontname);
			}

			if(cf == nil || msf == nil){
				fprint(2, "%C (%d) not found\n", *r, *r);
				len--, r++;
				continue;
			}
		}

		/* count chars we can draw in the subfont */
		for(n = 0; n < len; n++){
			if(r[n] == 0 || cf->min > r[n] || r[n] > cf->max)
				break;
		}

		for(i = 0; i < n; i++){
			idx = r[i] + cf->offset - cf->min;
			fc = &msf->info[idx];
			memimagedraw(dst, Rect(pt.x+fc->left, pt.y+fc->top, pt.x+fc->left+(fc[1].x-fc[0].x), pt.y+fc->bottom), src, sp, msf->bits, Pt(fc->x, fc->top), op);
			pt.x += fc->width;
		}

		freememsubfont(msf);
		msf = nil;

		len-=n, r+=n;
	}

	if(msf)
		freememsubfont(msf);

	return pt;
}

static void
strtobit(int fd, Font *f, ulong fg, ulong bg, char *s)
{
	Rune *r;
	Point txtpt;
	Memimage *fgi, *tmp, *dst;

	fgi = allocmemimage(Rect(0, 0, 1, 1), RGBA32);
	fgi->flags = Frepl;
	fgi->clipr = Rect(-0x3FFFFFF, -0x3FFFFFF, 0x3FFFFFF, 0x3FFFFFF);
	memfillcolor(fgi, fg);

	tmp = allocmemimage(Rect(0, 0, stringwidth(f, s), f->height*2), RGBA32);
	if(tmp == nil)
		sysfatal("allocmemimage: %r");

	memfillcolor(tmp, bg);

	r = runesmprint("%s", s);
	if(r == nil)
		sysfatal("runesmprint: %r");
	txtpt = memstr(tmp, ZP, fgi, ZP, f, r, 1<<24, SoverD);
	free(r);

	dst = allocmemimage(Rect(0, 0, txtpt.x, f->height + 1), RGBA32);
	if(dst == nil)
		sysfatal("allocmemimage: %r");

	memimagedraw(dst, dst->r, tmp, ZP, nil, ZP, S);
	freememimage(tmp);

	writeuncompressed(fd, dst);
	freememimage(dst);
}

static void
usage(void)
{
	fprint(2, "usage: %s [-f font] strings...\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *fname, *str;
	int i, len;
	ulong fg, bg;
	Font *txtfont;

	fname = nil;
	fg = DWhite;
	bg = DBlack;

	ARGBEGIN{
	case 'f':
		fname = strdup(EARGF(usage()));
		break;
	case 'F':
		fg = strtoul(EARGF(usage()), nil, 16);
		if(fg > 0xFFFFFF)
			sysfatal("fg %6luX out of range", fg);
		fg = (fg << 8) | 0xFF;
		break;
	case 'B':
		bg = strtoul(EARGF(usage()), nil, 16);
		if(bg > 0xFFFFFF)
			sysfatal("bg %6luX out of range", bg);
		bg = (bg << 8) | 0xFF;
		break;
	default:
		usage();
	}ARGEND

	if(argc < 1)
		usage();

	memimageinit();

	/* load font */
	if(fname == nil){
		fname = getenv("font");
	}

	txtfont = openfont(nil, fname);
	if(txtfont == nil)
		sysfatal("openfont: %r");

	free(fname);

	/* concatenate args */
	len = 0;

	for(i = 0; i < argc; i++){
		len += strlen(argv[i]);
	}

	str = mallocz(len+argc, 1);
	if(str == nil)
		sysfatal("malloc: %r");

	for(i = 0; i < argc; i++){
		strcat(str, argv[i]);
		if(i != argc-1)
			strcat(str, " ");
	}

	strtobit(2, txtfont, fg, bg, str);
	free(str);
	freefont(txtfont);

	/* no leaks! */
	freememimage(memwhite);
	freememimage(memblack);
}
