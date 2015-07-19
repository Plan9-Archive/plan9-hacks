#include <u.h>
#include <libc.h>
#include <libsec.h>

#include "fns.h"

/*
	echo -n "0123456789abcdef" | openssl enc -e -aes-128-cbc -bufsize 16 -nopad -nosalt -iv 2B95990A9151374ABD8FF8C5A7A0FE08 -K  5F4DCC3B5AA765D61D8327DEB882CF99 | xd -1

0000000  7e 9c 50 66 2b f2 e2 71 f4 e8 bc e2 3b 13 21 7e
*/


uchar *plaintext	= (uchar*) "0123456789abcdef";
uchar IV[16]	= {
	0x2B, 0x95, 0x99, 0x0A,
	0x91, 0x51, 0x37, 0x4A,
	0xBD, 0x8F, 0xF8, 0xC5,
	0xA7, 0xA0, 0xFE, 0x08,
};

uchar key[16]	= {
	0x5F, 0x4D, 0xCC, 0x3B,
	0x5A, 0xA7, 0x65, 0xD6,
	0x1D, 0x83, 0x27, 0xDE,
	0xB8, 0x82, 0xCF, 0x99,
};

int keysize		= 16;

void
dump(uchar *p, int len) {
	int i;
	for(i = 0; i < len; i++) {
		print("%2.2x ", p[i]);
	}
}

void
aesniCBCencrypt(uchar *p, int len, AESstate *s)
{
	uchar *p2, *ip, *eip;
	uchar q[AESbsize];

	for(; len >= AESbsize; len -= AESbsize){
		p2 = p;
		ip = s->ivec;
		for(eip = ip+AESbsize; ip < eip; )
			*p2++ ^= *ip++;
		_aesni_encrypt(s->ekey, s->rounds, p, q);
		memmove(s->ivec, q, AESbsize);
		memmove(p, q, AESbsize);
		p += AESbsize;
	}

	if(len > 0){
		ip = s->ivec;
		_aesni_encrypt(s->ekey, s->rounds, ip, q);
		memmove(s->ivec, q, AESbsize);
		for(eip = ip+len; ip < eip; )
			*p++ ^= *ip++;
	}
}

void
main(void) {
	uchar enc[16];
	AESstate cbc;
	setupAESstate(&cbc, key, keysize, IV);

	print("aes_encrypt:\t");
	memcpy(enc, plaintext, 16);
	aesCBCencrypt(enc, 16, &cbc);
	dump(enc, 16);
	print("\n");

	setupAESstate(&cbc, key, keysize, IV);

	print("_aesni_encrypt:\t");
	aesniCBCencrypt(enc, 16, &cbc);
	dump(enc, 16);
	print("\n");
	return;
}
