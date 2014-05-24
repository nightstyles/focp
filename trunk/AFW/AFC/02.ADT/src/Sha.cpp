
#include "Binary.hpp"

FOCP_BEGIN();

#define K1 0x5A827999
#define K2 0x6ED9EBA1
#define K3 0x8F1BBCDC
#define K4 0xCA62C1D6

#define F1(B, C, D) ((B & C) | (~B & D))
#define F2(B, C, D) (B ^ C ^ D)
#define F3(B, C, D) ((B & C) | (B & D) | (C & D))
#define F4(B, C, D) (B ^ C ^ D)

#define ROL(A, K) ((A << K) | (A >> (32 - K)))

static uchar padding[64] =
{
	0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  /*  8 */
	0x00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  /* 16 */
	0x00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  /* 24 */
	0x00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  /* 32 */
	0x00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  /* 40 */
	0x00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  /* 48 */
	0x00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  /* 56 */
	0x00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0   /* 64 */
};

static bool g_bNeedCode = (CBinary::U32Code(1) != 1);

static void SHATransform(CShaContext *ctx, const uchar *X)
{
	uint32 a, b, c, d, e, temp = 0;
	uint32 W[80]; /* Work array for SHS    */
	int32 i;

	if(g_bNeedCode == false)
		CBinary::MemoryCopy(W, X, 64);
	else for (i = 0; i < 64; i += 4)
			W[(i/4)] = ((uint32)X[i+3]) | (((uint32)X[i+2]) << 8) | (((uint32)X[i+1]) << 16) | (((uint32)X[i]) << 24);
	for (i = 16; i < 80; i++)
		W[i] = ROL((W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16]), 1);
	a = ctx->h[0];
	b = ctx->h[1];
	c = ctx->h[2];
	d = ctx->h[3];
	e = ctx->h[4];

	for (i =  0; i <= 19; i++)
	{
		temp = ROL(a, 5) + F1(b, c, d) + e + K1 + W[i];
		e = d;
		d = c;
		c = ROL(b, 30);
		b = a;
		a = temp;
	}

	for (i = 20; i <= 39; i++)
	{
		temp = ROL(a, 5) + F2(b, c, d) + e + K2 + W[i];
		e = d;
		d = c;
		c = ROL(b, 30);
		b = a;
		a = temp;
	}

	for (i = 40; i <= 59; i++)
	{
		temp = ROL(a, 5) + F3(b, c, d) + e + K3 + W[i];
		e = d;
		d = c;
		c = ROL(b, 30);
		b = a;
		a = temp;
	}

	for (i = 60; i <= 79; i++)
	{
		temp = ROL(a, 5) + F4(b, c, d) + e + K4 + W[i];
		e = d;
		d = c;
		c = ROL(b, 30);
		b = a;
		a = temp;
	}

	ctx->h[0] += a;
	ctx->h[1] += b;
	ctx->h[2] += c;
	ctx->h[3] += d;
	ctx->h[4] += e;
}

ADT_API void InitSha(CShaContext *ctx)
{
	/* Zero the SHS Context */
	CBinary::MemorySet(ctx, 0, sizeof(*ctx));

	/* Prime the SHS with "magic" init constants */
	ctx->h[0] = 0x67452301;
	ctx->h[1] = 0xEFCDAB89;
	ctx->h[2] = 0x98BADCFE;
	ctx->h[3] = 0x10325476;
	ctx->h[4] = 0xC3D2E1F0;
}

ADT_API void UpdateSha(CShaContext *ctx, const uchar *buf, uint32 lenBuf)
{
	/* Do we have any bytes? */
	if (lenBuf == 0)
		return;

	/* Calculate buf len in bits and update the len count */
	ctx->count[0] += (lenBuf << 3);
	if (ctx->count[0] < (lenBuf << 3))
		ctx->count[1] += 1;
	ctx->count[1] += (lenBuf >> 29);

	/* Fill the hash working buffer for the first run, if  */
	/* we have enough data...                              */
	int32 i = 64 - ctx->index;  /* either fill it up to 64 bytes */
	if ((int32)lenBuf < i)
		i = lenBuf; /* or put the whole data...*/

	lenBuf -= i;  /* Reflect the data we'll put in the buf */

	/* Physically put the data in the hash workbuf */
	CBinary::MemoryCopy(&(ctx->X[ctx->index]), buf, i);
	buf += i;
	ctx->index += i;

	/* Adjust the buf index */
	if (ctx->index == 64)
		ctx->index = 0;

	/* Let's see whether we're equal to 64 bytes in buf  */
	if (ctx->index == 0)
		SHATransform(ctx, ctx->X);

	/* Process full 64-byte blocks */
	while(lenBuf >= 64)
	{
		lenBuf -= 64;
		SHATransform(ctx, buf);
		buf += 64;
	}

	/* Put the rest of data in the hash buf for next run */
	if (lenBuf > 0)
	{
		CBinary::MemoryCopy(ctx->X, buf, lenBuf);
		ctx->index = lenBuf;
	}
}

ADT_API void EndSha(CShaContext *ctx, uchar digest[20])
{
	int32 i;
	uint32 c0, c1;
	uchar truelen[8];

	if(g_bNeedCode == false)
	{
		CBinary::MemoryCopy(truelen, &ctx->count[1], 4);
		CBinary::MemoryCopy(&truelen[4], &ctx->count[0], 4);
	}
	else
	{
		c0 = ctx->count[0];
		c1 = ctx->count[1];
		for (i = 7; i >=0; i--)
		{
			truelen[i] = (uchar) (c0 & 0xff);
			c0 = (c0 >> 8) | (((c1 >> 8) & 0xff) << 24);
			c1 = (c1 >> 8);
		}
	}

	/* How many padding bytes do we need? */
	i = (ctx->count[0] >> 3) & 0x3f;  /* # of bytes mod 64 */
	if (i >= 56)
		i = 120 - i; /* # of padding bytes needed */
	else
		i = 56 - i;

	UpdateSha(ctx, padding, i);   /* Append the padding */
	UpdateSha(ctx, truelen, 8);   /* Append the length  */

	if(g_bNeedCode == false)
		CBinary::MemoryCopy(digest, &ctx->h[0], 20);
	else for (i = 0; i < 4; i++)
		{
			digest[3-i]  = (uchar) (ctx->h[0] & 0xff); ctx->h[0] >>= 8;
			digest[7-i]  = (uchar) (ctx->h[1] & 0xff); ctx->h[1] >>= 8;
			digest[11-i] = (uchar) (ctx->h[2] & 0xff); ctx->h[2] >>= 8;
			digest[15-i] = (uchar) (ctx->h[3] & 0xff); ctx->h[3] >>= 8;
			digest[19-i] = (uchar) (ctx->h[4] & 0xff); ctx->h[4] >>= 8;
		}
}

ADT_API void GetSha(uchar sOutput[20], const uchar *sInput, uint32 nInLen)
{
	CShaContext oCtx;
	InitSha(&oCtx);
	UpdateSha(&oCtx, sInput, nInLen);
	EndSha(&oCtx, sOutput);
}

FOCP_END();
