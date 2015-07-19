#include <u.h>
#include <authsrv.h>
#include <libc.h>

#define	CHAR(x)		*p++ = f->x
#define	SHORT(x)	p[0] = f->x; p[1] = f->x>>8; p += 2
#define	VLONG(q)	p[0] = (q); p[1] = (q)>>8; p[2] = (q)>>16; p[3] = (q)>>24; p += 4
#define	LONG(x)		VLONG(f->x)
#define	STRING(x,n)	memmove(p, f->x, n); p += n

int
convT2M(Ticket *f, char *ap, char *key)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	CHAR(num);
	STRING(chal, CHALLEN);
	STRING(cuid, ANAMELEN);
	STRING(suid, ANAMELEN);
	STRING(key, DESKEYLEN);
	n = p - (uchar*)ap;
	if(key)
		encrypt(key, ap, n);
	return n;
}

void
main(int argc, char **argv)
{
	char buf[TICKETLEN];
	Ticket t;
	memset(&t, 0, sizeof(Ticket));

	t.num = 1;
	memmove(t.chal, "12345678", 8);
	memmove(t.cuid, "12345678", 8);
	memmove(t.suid, "12345678", 8);
	memmove(t.key, "12345678", 8);

	convT2M(&t, buf, "12345678");

	fmtinstall('H', encodefmt);

	print("%.*H\n", TICKETLEN, buf);

	USED(argc);
	USED(argv);
}
