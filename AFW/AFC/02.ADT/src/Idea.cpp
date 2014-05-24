
#include "AdtDef.hpp"

FOCP_BEGIN();

#define GET_16BIT(cp) (((uint16)(uchar)(cp)[0] << 8) | \
						((uint16)(uchar)(cp)[1]))
#define GET_32BIT(cp) (((uint32)(uchar)(cp)[0] << 24) | \
  						((uint32)(uchar)(cp)[1] << 16) | \
						((uint32)(uchar)(cp)[2] << 8) | \
						((uint32)(uchar)(cp)[3]))
#define PUT_32BIT(cp, value) \
do { \
  (cp)[0] = (value) >> 24; \
  (cp)[1] = (value) >> 16; \
  (cp)[2] = (value) >> 8; \
  (cp)[3] = (value); \
} while (0)


ADT_API void SetIdeaKey(CIdeaContext* c, const uchar key[16])
{
	int32 i;
	uint16 *keys;

	/* Get pointer to the keys. */
	keys = c->key_schedule;

	/* Keys for the first round are directly taken from the user-supplied key. */
	for (i = 0; i < 8; i++)
		keys[i] = GET_16BIT(key + 2 * i);

	/* Each round uses the key of the previous key, rotated to the left by 25
	bits.  The last four keys (output transform) are the first four keys
	from what would be the ninth round. */
	for (i = 8; i < 52; i++)
	{
		if ((i & 7) == 0)
			keys += 8;
		keys[i & 7] = ((keys[((i + 1) & 7) - 8] << 9) | (keys[((i + 2) & 7) - 8] >> 7)) & 0xffff;
	}
}

static inline uint32 mulop(uint32 a, uint32 b)
{
	uint32 ab = a * b;
	if (ab != 0)
	{
		uint32 lo = ab & 0xffff;
		uint32 hi = (ab >> 16) & 0xffff;
		return (lo - hi) + (lo < hi);
	}
	if (a == 0)
		return 1 - b;
	return  1 - a;
}

static void UpdateIdea(CIdeaContext* c, uint32 l, uint32 r, uint32 output[2])
{
	uint32 round;
	uint16 *keys;
	uint32 t1, t2, x1, x2, x3, x4;

	keys = c->key_schedule;
	x1 = l >> 16;
	x2 = l;
	x3 = r >> 16;
	x4 = r;
	for (round = 0; round < 8; round++)
	{
		x1 = mulop(x1 & 0xffff, keys[0]);
		x3 = x3 + keys[2];
		x4 = mulop(x4 & 0xffff, keys[3]);
		x2 = x2 + keys[1];
		t1 = x1 ^ x3;
		t2 = x2 ^ x4;
		t1 = mulop(t1 & 0xffff, keys[4]);
		t2 = t1 + t2;
		t2 = mulop(t2 & 0xffff, keys[5]);
		t1 = t1 + t2;
		x1 = x1 ^ t2;
		x4 = x4 ^ t1;
		t1 = t1 ^ x2;
		x2 = t2 ^ x3;
		x3 = t1;
		keys += 6;
	}

	x1 = mulop(x1 & 0xffff, keys[0]);
	x3 = (x2 + keys[2]) & 0xffff;
	x2 = t1 + keys[1]; /* t1 == old x3 */
	x4 = mulop(x4 & 0xffff, keys[3]);
	output[0] = (x1 << 16) | (x2 & 0xffff);
	output[1] = (x3 << 16) | (x4 & 0xffff);
}

/*
ADT_API void IdeaEncode(CIdeaContext* c, uchar iv[8], uchar* dest, const uchar* src, uint32 len)
{
	uint32 iv0, iv1, out[2];
	uint32 i;

	iv0 = GET_32BIT(iv);
	iv1 = GET_32BIT(iv + 4);

	for (i = 0; i < len; i += 8)
	{
		UpdateIdea(c, iv0, iv1, out);
		iv0 = out[0] ^ GET_32BIT(src + i);
		iv1 = out[1] ^ GET_32BIT(src + i + 4);
		if (i + 8 <= len)
		{
			PUT_32BIT(dest + i, iv0);
			PUT_32BIT(dest + i + 4, iv1);
		}
		else
		{
			switch (len - i)
			{
			case 7:	dest[i + 6] = iv1 >> 8;
			case 6:	dest[i + 5] = iv1 >> 16;
			case 5:	dest[i + 4] = iv1 >> 24;
			case 4:	dest[i + 3] = iv0;
			case 3:	dest[i + 2] = iv0 >> 8;
			case 2:	dest[i + 1] = iv0 >> 16;
			case 1:	dest[i] = iv0 >> 24;
			}
		}
	}
	PUT_32BIT(iv, iv0);
	PUT_32BIT(iv + 4, iv1);
}
*/

ADT_API void MakeIdea(CIdeaContext* c, uchar iv[8], uchar* dest, const uchar* src, uint32 len, bool enc)
{
	uint32 iv0, iv1, out[2];
	uint32 i;

	iv0 = GET_32BIT(iv);
	iv1 = GET_32BIT(iv + 4);

	for (i = 0; i < len; i += 8)
	{
		UpdateIdea(c, iv0, iv1, out);
		iv0 = GET_32BIT(src + i);
		iv1 = GET_32BIT(src + i + 4);
		uint32 plain0 = out[0] ^ iv0;
		uint32 plain1 = out[1] ^ iv1;
		if (i + 8 <= len)
		{
			PUT_32BIT(dest + i, plain0);
			PUT_32BIT(dest + i + 4, plain1);
		}
		else
		{
			switch (len - i)
			{
			case 7:	dest[i + 6] = plain1 >> 8;
			case 6:	dest[i + 5] = plain1 >> 16;
			case 5:	dest[i + 4] = plain1 >> 24;
			case 4:	dest[i + 3] = plain0;
			case 3:	dest[i + 2] = plain0 >> 8;
			case 2:	dest[i + 1] = plain0 >> 16;
			case 1:	dest[i] = plain0 >> 24;
			}
		}
		if(enc)
		{
			iv0 = plain0;
			iv1 = plain1;
		}
	}

	PUT_32BIT(iv, iv0);
	PUT_32BIT(iv + 4, iv1);
}

FOCP_END();
