#include <u.h>

// returns 1 if x == y, otherwise 0.
int
constbeq(uchar x, uchar y) {
	uchar z;

	z = ~(x ^ y);
        z &= (z >> 4);
        z &= (z >> 2);
        z &= (z >> 1);

	return z;
}

// returns 1 if the the len bytes in x are equal to len bytes in y,
// otherwise returns 0.
int
constcmp(uchar *x, uchar *y, int len) {
	uchar z;
	int i;

	z = 0;

	for(i = 0; i < len; i++) {
		z |= x[i] ^ y[i];
	}

	return constbeq(z, 0);
}
