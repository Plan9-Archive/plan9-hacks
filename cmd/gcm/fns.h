void setupAESGCMstate(AESstate *g);
void aesGCMencrypt(AESstate *g, uchar *dst, uchar *nonce, uchar *pt, int ptlen, uchar *data, int dlen);
int aesGCMdecrypt(AESstate *g, uchar *dst, uchar *nonce, uchar *ct, int ctlen, uchar *data, int dlen);

enum
{
	GCMBlockSize = 16,
	GCMTagSize = 16,
	GCMNonceSize = 12,
};
