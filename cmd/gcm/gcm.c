#include <u.h>
#include <libc.h>
#include <libsec.h>
#include "fns.h"

static u64int
rev(u64int i)
{
	i = ((i << 2) & 0xc) | ((i >> 2) & 0x3);
	i = ((i << 1) & 0xa) | ((i >> 1) & 0x5);

	return i;
}

static void
xorb(uchar *dst, uchar *a, uchar *b, int len)
{
	int i;
	for(i = 0; i < len; i++) {
		dst[i] = a[i] ^ b[i];
	}
}

static u64int
getu64int(uchar *in)
{
	u64int out;

	out = (u64int)in[0]<<56 |
		(u64int)in[1] << 48 |
		(u64int)in[2] << 40 |
		(u64int)in[3] << 32 |
		(u64int)in[4] << 24 |
		(u64int)in[5] << 16 |
		(u64int)in[6] << 8 |
		(u64int)in[7];

	return out;
}

static void
putu64int(uchar *out, u64int in)
{
	out[0] = (u8int) (in >> 56);
	out[1] = (u8int) (in >> 48);
	out[2] = (u8int) (in >> 40);
	out[3] = (u8int) (in >> 32);
	out[4] = (u8int) (in >> 24);
	out[5] = (u8int) (in >> 16);
	out[6] = (u8int) (in >> 8);
	out[7] = (u8int) in;
}

static GCMFieldElement
gcmadd(GCMFieldElement *x, GCMFieldElement *y)
{
	GCMFieldElement z;

	z.low = x->low ^ y->low;
	z.high = x->high ^ y->high;

	return z;
}

static GCMFieldElement
gcmdouble(GCMFieldElement *x)
{
	GCMFieldElement z;
	int msbset;

	msbset = x->high & 1;

	z.high = x->high >> 1;
	z.high |= x->low << 63;
	z.low = x->low >> 1;

	if(msbset)
		z.low ^= 0xe100000000000000ull;

	return z;
}

static u16int gcmReductionTable[] = {
	0x0000, 0x1c20, 0x3840, 0x2460, 0x7080, 0x6ca0, 0x48c0, 0x54e0,
	0xe100, 0xfd20, 0xd940, 0xc560, 0x9180, 0x8da0, 0xa9c0, 0xb5e0,
};

static void
gcmmul(AESstate *g, GCMFieldElement *y)
{
	GCMFieldElement z, *t;
	u64int word, msw;
	int i, j;

	memset(&z, 0, sizeof(z));

	for (i = 0; i < 2; i++) {
		word = y->high;

		if(i == 1)
			word = y->low;

		for(j = 0; j < 64; j += 4) {
			msw = z.high & 0xF;

			z.high >>= 4;
			z.high |= z.low << 60;
			z.low >>= 4;
			z.low ^= ((u64int)gcmReductionTable[msw]) << 48;

			t = &g->producttable[word & 0xF];

			z.low ^= t->low;
			z.high ^= t->high;
			word >>= 4;
		}
	}

	*y = z;
}

static void
gcmUpdateBlocks(AESstate *g, GCMFieldElement *y, uchar *blocks, int len)
{
	while(len > 0) {
		y->low ^= getu64int(blocks);
		y->high ^= getu64int(blocks+8); // +sizeof(u64int)
		gcmmul(g, y);
		blocks += GCMBlockSize;
		len -= GCMBlockSize;
	}
}

static void
gcmUpdate(AESstate *g, GCMFieldElement *y, uchar *blocks, int len)
{
	int fullblocks;
	uchar partial[GCMBlockSize];

	fullblocks = (len >> 4) << 4;

	gcmUpdateBlocks(g, y, blocks, fullblocks);

	if(len != fullblocks) {
		memset(partial, 0, GCMBlockSize);
		memcpy(partial, blocks + fullblocks, len - fullblocks);
		gcmUpdateBlocks(g, y, partial, GCMBlockSize);
	}
}

static void
gcmInc32(uchar counterblock[GCMBlockSize])
{
	int i;

	for(i = GCMBlockSize - 1; i >= GCMBlockSize - 4; i--) {
		counterblock[i]++;
		if(counterblock[i] != 0) {
			break;
		}
	}
}

static void
gcmAuth(AESstate *g, uchar *out, int outlen, uchar *ciphertext, int cipherlen, uchar *additional, int alen, uchar tagmask[GCMTagSize])
{
	USED(outlen);

	GCMFieldElement y;

	memset(&y, 0, sizeof(y));

	gcmUpdate(g, &y, additional, alen);
	gcmUpdate(g, &y, ciphertext, cipherlen);

	y.low ^= (u64int)alen * 8;
	y.high ^= (u64int)cipherlen * 8;

	gcmmul(g, &y);

	putu64int(out, y.low);
	putu64int(out+8, y.high); // +sizeof(u64int)

	xorb(out, out, tagmask, GCMTagSize);
}

static void
gcmCounterCrypt(AESstate *g, uchar *out, uchar *in, int inlen, uchar counter[GCMBlockSize])
{
	uchar mask[GCMBlockSize];

	while(inlen >= GCMBlockSize) {
		aes_encrypt(g->ekey, g->rounds, counter, mask);
		gcmInc32(counter);

		xorb(out, in, mask, GCMBlockSize);

		out += GCMBlockSize;
		in += GCMBlockSize;
		inlen -= GCMBlockSize;
	}

	if(inlen > 0) {
		aes_encrypt(g->ekey, g->rounds, counter, mask);
		gcmInc32(counter);
		xorb(out, in, mask, inlen);
	}
}

void
setupAESGCMstate(AESstate *g)
{
	GCMFieldElement x;
	uchar key[GCMBlockSize];
	int i;

	memset(key, 0, sizeof(key));

	aes_encrypt(g->ekey, g->rounds, key, key);

	x.low = getu64int(key);
	x.high = getu64int(key+8); // +sizeof(u64int)

	g->producttable[rev(1)] = x;

	for(i = 2; i < 16; i += 2) {
		g->producttable[rev(i)] = gcmdouble(&g->producttable[rev(i/2)]);
		g->producttable[rev(i+1)] = gcmadd(&g->producttable[rev(i)], &x);
	}
}

// dst must be len(plaintext)+GCMTagSize
// nonce must be GCMNonceSize
void
aesGCMencrypt(AESstate *g, uchar *dst, uchar *nonce, uchar *pt, int ptlen, uchar *data, int dlen)
{
	uchar counter[GCMBlockSize], tagMask[GCMBlockSize];

	memset(counter, 0, GCMBlockSize);
	memset(tagMask, 0, GCMBlockSize);

	memcpy(counter, nonce, GCMNonceSize);
	counter[GCMBlockSize-1] = 1;

	aes_encrypt(g->ekey, g->rounds, counter, tagMask);
	gcmInc32(counter);

	gcmCounterCrypt(g, dst, pt, ptlen, counter);
	gcmAuth(g, dst+ptlen, GCMTagSize, dst, ptlen, data, dlen, tagMask);
}

// returns -1 if decryption *or* authentication fails.
int
aesGCMdecrypt(AESstate *g, uchar *dst, uchar *nonce, uchar *ct, int ctlen, uchar *data, int dlen)
{
	uchar *tag, tagMask[GCMBlockSize], expectedTag[GCMTagSize], counter[GCMBlockSize];

	memset(tagMask, 0, GCMBlockSize);
	memset(expectedTag, 0, GCMTagSize);
	memset(counter, 0, GCMBlockSize);

	tag = ct + ctlen - GCMTagSize;
	ctlen -= GCMTagSize;

	memcpy(counter, nonce, GCMNonceSize);
	counter[GCMBlockSize-1] = 1;

	aes_encrypt(g->ekey, g->rounds, counter, tagMask);
	gcmInc32(counter);

	gcmAuth(g, expectedTag, GCMBlockSize, ct, ctlen, data, dlen, tagMask);

	if(constcmp(expectedTag, tag, GCMTagSize) != 1) {
		return -1;
	}

	gcmCounterCrypt(g, dst, ct, ctlen, counter);

	return 0;
}
