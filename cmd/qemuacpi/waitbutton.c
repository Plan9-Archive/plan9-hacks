#include <u.h>
#include <libc.h>
#include <aml.h>

int nflag;
int fd, iofd;
ulong PM1a_EVT_BLK, PM1b_EVT_BLK;
ulong PM1a_CNT_BLK, PM1b_CNT_BLK;
ulong PM1a_EN, PM1b_EN;

enum {
	PWRBTN_EN	= (1<<8),
	PWRBTN_STS	= (1<<8),
};

typedef struct Tbl Tbl;
struct Tbl {
	uchar	sig[4];
	uchar	len[4];
	uchar	rev;
	uchar	csum;
	uchar	oemid[6];
	uchar	oemtid[8];
	uchar	oemrev[4];
	uchar	cid[4];
	uchar	crev[4];
	uchar	data[];
};

typedef struct FADT FADT;
struct FADT
{
	uchar	sig[4];
	uchar	len[4];
	uchar	rev;
	uchar	csum;
	uchar	oemid[6];
	uchar	oemtid[8];
	uchar	oemrev[4];
	uchar	cid[4];
	uchar	crev[4];
	uchar	firmwarectl[4];
	uchar	dsdt[4];
	uchar	reserved1;
	uchar	preferredpmprofile;
	uchar	sciint[2];
	uchar	scicmd[4];
	uchar	acpienable;
	uchar	acpidisable;
	uchar	s4biosreq;
	uchar	pstatecnt;
	uchar	pm1aevtblk[4];
	uchar	pm1bevtblk[4];
	uchar	pm1acntblk[4];
	uchar	pm1bcntblk[4];
	uchar	pm2cntblk[4];
	uchar	pmtmrblk[4];
	uchar	gpe0blk[4];
	uchar	gpe1blk[4];
	uchar	pm1evtlen;
	uchar	pm1cntlen;
	uchar	pm2cntlen;
	uchar	pmtmrlen;
	uchar	gpe0blklen;
	uchar	gpe1blklen;
	uchar	gpe1base;
	uchar	cstcnt;
	uchar	plvl2lat[2];
	uchar	plvl3lat[2];
	uchar	flushsize[2];
	uchar	flushstride[2];
	uchar	dutyoffset;
	uchar	dutywidth;
	uchar	dayalarm;
	uchar	monalarm;
	uchar	century;
	uchar	iapcbootarch[2];
	uchar	reserved2;
	uchar	flags[4];
};

enum {
	Tblsz	= 4+4+1+1+6+8+4+4+4,
};

static ushort
get16(uchar *p)
{
	return p[1] << 8 | p[0];
}

static ulong
get32(uchar *p){
	return p[3]<<24 | p[2]<<16 | p[1]<<8 | p[0];
}

#pragma	varargck	type	"F"	FADT*

int
Ffmt(Fmt *f)
{
	FADT *fadt;

	fadt = va_arg(f->args, FADT*);
	fmtprint(f, "== FADT ==\n");
	fmtprint(f, "\tlen: %lud\n", get32(fadt->len));
	fmtprint(f, "\tsciint: %hud\n", get16(fadt->sciint));
	fmtprint(f, "\tscicmd: %#ux\n", get16(fadt->scicmd));
	fmtprint(f, "\tacpienable: %#hhux\n", fadt->acpienable);
	fmtprint(f, "\tacpidisable: %#hhux\n", fadt->acpidisable);
	fmtprint(f, "\ts4biosreq:	%#hhux\n", fadt->s4biosreq);
	if(get32(fadt->pm1aevtblk) > 0)
		fmtprint(f, "\tpm1aevtblk: %#lux-%#lux\n", get32(fadt->pm1aevtblk), get32(fadt->pm1aevtblk) + fadt->pm1evtlen - 1);
	if(get32(fadt->pm1bevtblk) > 0)
		fmtprint(f, "\tpm1bevtblk: %#lux-%#lux\n", get32(fadt->pm1bevtblk), get32(fadt->pm1bevtblk) + fadt->pm1evtlen - 1);
	if(get32(fadt->pm1acntblk) > 0)
		fmtprint(f, "\tpm1acntblk: %#lux-%#lux\n", get32(fadt->pm1acntblk), get32(fadt->pm1acntblk) + fadt->pm1cntlen - 1);
	if(get32(fadt->pm1bcntblk) > 0)
		fmtprint(f, "\tpm1bcntblk: %#lux-%#lux\n", get32(fadt->pm1bcntblk), get32(fadt->pm1bcntblk) + fadt->pm1cntlen - 1);
	fmtprint(f, "\tflags: %lub\n", get32(fadt->flags));
	return fmtprint(f, "==========\n");
}

int
loadacpi(void)
{
	ulong l;
	int n;
	Tbl *t;
	FADT *fadt;

	amlinit();
	for(;;){
		t = malloc(sizeof(*t));
		if((n = readn(fd, t, Tblsz)) <= 0)
			break;
		if(n != Tblsz)
			return -1;
		l = get32(t->len);
		if(l < Tblsz)
			return -1;
		l -= Tblsz;
		t = realloc(t, sizeof(*t) + l);
		if(readn(fd, t->data, l) != l)
			return -1;
		if(memcmp("DSDT", t->sig, 4) == 0)
			amlload(t->data, l);
		else if(memcmp("SSDT", t->sig, 4) == 0)
			amlload(t->data, l);
		else if(memcmp("FACP", t->sig, 4) == 0){
			fadt = (FADT*) t;
			//print("%F", fadt);
			PM1a_EVT_BLK = get32(fadt->pm1aevtblk);
			PM1b_EVT_BLK = get32(fadt->pm1bevtblk);
			PM1a_CNT_BLK = get32(fadt->pm1acntblk);
			PM1b_CNT_BLK = get32(fadt->pm1bcntblk);
			PM1a_EN = PM1a_EVT_BLK + (fadt->pm1evtlen / 2);
			PM1b_EN = PM1b_EVT_BLK + (fadt->pm1evtlen / 2);

			/*
			print("PM1a_EVT_BLK %#lux\n", PM1a_EVT_BLK);
			print("PM1b_EVT_BLK %#lux\n", PM1b_EVT_BLK);
			print("PM1a_CNT_BLK %#lux\n", PM1a_CNT_BLK);
			print("PM1b_CNT_BLK %#lux\n", PM1b_CNT_BLK);
			print("PM1a_EN %#lux\n", PM1a_EN);
			print("PM1b_EN %#lux\n", PM1b_EN);
			*/
		}
	}
	return 0;
}

void
outw(long addr, ushort val)
{
	uchar buf[2];

	if(addr == 0)
		return;
	buf[0] = val;
	buf[1] = val >> 8;
	pwrite(iofd, buf, 2, addr);
}

ushort
inw(long addr)
{
	uchar buf[2];

	if(addr == 0)
		return 0;

	pread(iofd, buf, 2, addr);
	return (ushort)(buf[0] | buf[1] << 8);
}

void
wirecpu0(void)
{
	char buf[128];
	int ctl;

	snprint(buf, sizeof(buf), "/proc/%d/ctl", getpid());
	if((ctl = open(buf, OWRITE)) < 0){
		snprint(buf, sizeof(buf), "#p/%d/ctl", getpid());
		if((ctl = open(buf, OWRITE)) < 0)
			return;
	}
	write(ctl, "wired 0", 7);
	close(ctl);
}

void
usage(void)
{
	fprint(2, "usage: %s [-n]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	ushort sts;

	ARGBEGIN{
	case 'n':
		nflag++;
		break;
	default:
		usage();
	}ARGEND

	fmtinstall('F', Ffmt);
	wirecpu0();

	if((fd = open("/dev/acpitbls", OREAD)) < 0)
		if((fd = open("#P/acpitbls", OREAD)) < 0)
			goto fail;
	if((iofd = open("/dev/iow", ORDWR)) < 0)
		if((iofd = open("#P/iow", ORDWR)) < 0)
			goto fail;
	if(loadacpi() < 0)
		goto fail;

	/* qemu seems to need this, contrary to the spec */
	outw(PM1a_EN, PWRBTN_EN);

	/* wait for the power button bit to be set */
	for(;;){
		sleep(100);
		sts = inw(PM1a_EVT_BLK);
		//fprint(2, "PM1x_EVT_BLK: %.16hub powerbutton ? %s\n", sts, sts & PWRBTN_STS ? "true":"false");
		if(sts & PWRBTN_STS)
			break;
	}

	/* clear the bit */
	if(!nflag)
		outw(PM1a_EVT_BLK, sts);

	exits(nil);
fail:
	sysfatal("%s: %r", argv0);
}

