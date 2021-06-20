#include "MiniRangeCoder.h"
#include <assert.h>
#include <string.h>

#define RANGE_SHIFT		11
#define NOT_COMPRESS	0xFF
#define FIRST_RANGE		0xFFFFFFFF

static_assert((1 << RANGE_SHIFT) == MAX_TOTAL_FREQ, "failed MAX_TOTAL_FREQ or RANGE_SHIFT");

typedef struct
{
	uint8_t originalSize;
	uint8_t compressedSize;
} HEADER;

uint16_t RangeCoderEncode(const uint8_t* pSrc, uint8_t srcSize, uint8_t* pCompressed, FREQ_LOWER table[0x100])
{
	uint32_t range = FIRST_RANGE;
	uint32_t lower = 0;

	const uint8_t* pLast = pSrc + srcSize;

	HEADER* pCompressedHeader = (HEADER*)pCompressed;
	pCompressed += sizeof(HEADER);

	pCompressedHeader->originalSize = srcSize;

	uint8_t* pLimitCompressed = pCompressed + srcSize;

	if (srcSize <= 1)
		goto not_compress;

	goto start;

	while( pSrc != pLast)
	{
		if (range == 0)
			goto not_compress;	//Detect bytes with a frequency of 0

		while (range < 0x01000000)
		{
			*(pCompressed++) = (uint8_t)(lower >> 24);
			range <<= 8;
			lower <<= 8;

			if (pLimitCompressed == pCompressed)
				goto not_compress;
		}

	start:;

		uint8_t byte = *(pSrc++);

		uint32_t r = range >> RANGE_SHIFT;
		uint32_t l = r * table[byte].lower;

		range  = r * table[byte].freq;

		uint32_t n = lower + l;
		bool carryup = n < lower;
		lower = n;

		if (!carryup)
			continue;

		//carry up
		int i = -1;
		while (++pCompressed[i] == 0)
			--i;
	}

	if (range == 0)
		goto not_compress;	//Detect bytes with a frequency of 0

	for (int i = 0; i < 4 && lower; ++i)
	{
		*(pCompressed++) = (uint8_t)(lower >> 24);
		lower <<= 8;

		if (pLimitCompressed == pCompressed)
			goto not_compress;
	}

	uint32_t dstSize = (uintptr_t)pCompressed - (uintptr_t)pCompressedHeader;
	pCompressedHeader->compressedSize = (uint8_t)(dstSize - sizeof(HEADER));

	return dstSize;

not_compress:

	//Copy the original data since it could not be compressed.
	pCompressedHeader->compressedSize = NOT_COMPRESS;
	memcpy((void*)(pCompressedHeader + 1), (void*)(pLast - pCompressedHeader->originalSize), pCompressedHeader->originalSize);

	return sizeof(HEADER) + pCompressedHeader->originalSize;
}

bool RangeCoderDecode(const uint8_t* pCompressed, uint8_t* pOriginal, uint8_t* pOutOriginalSize, FREQ_LOWER table[0x100])
{
	uint32_t range = FIRST_RANGE;
	uint32_t lower = 0;

	HEADER* pHeader = (HEADER*)pCompressed;
	pCompressed += sizeof(HEADER);
	*pOutOriginalSize = pHeader->originalSize;

	if (pHeader->compressedSize != NOT_COMPRESS && pHeader->compressedSize >= pHeader->originalSize)
		goto corrupted;

	if (pHeader->compressedSize != NOT_COMPRESS)
	{
		const uint8_t* pCompressedLast = pCompressed + pHeader->compressedSize;

		for (int i = 0; i < 4; ++i)
		{
			lower <<= 8;
			if (pCompressed != pCompressedLast)
				lower += *(pCompressed++);
		}

		uint8_t* pOriginalLast = pOriginal + pHeader->originalSize;
		
		while (pOriginal != pOriginalLast)
		{
			uint32_t byte = 0;

			range >>= RANGE_SHIFT;

			if (table[byte + 128].lower * range <= lower) byte += 128;
			if (table[byte + 64].lower * range <= lower) byte += 64;
			if (table[byte + 32].lower * range <= lower) byte += 32;
			if (table[byte + 16].lower * range <= lower) byte += 16;
			if (table[byte + 8].lower * range <= lower) byte += 8;
			if (table[byte + 4].lower * range <= lower) byte += 4;
			if (table[byte + 2].lower * range <= lower) byte += 2;
			if (table[byte + 1].lower * range <= lower) byte += 1;

			*(pOriginal++) = byte;

			lower = lower - range * table[byte].lower;
			range = table[byte].freq * range;

			if (range < lower)
				goto corrupted;

			while (range < 0x01000000)
			{
				range <<= 8;
				lower <<= 8;

				if (pCompressed != pCompressedLast)
					lower += *(pCompressed++);
			}
		}

		return true;
	}

	//Copy the original data since it could not be compressed.
	memcpy(pOriginal, (void*)(pHeader + 1), pHeader->originalSize);
	return true;

corrupted:

	return false;
}

uint8_t GetOriginalSize( const uint8_t* pCompressed )
{
    HEADER* pHeader = (HEADER*)pCompressed;
    return pHeader->originalSize;
}

uint16_t GetDataSize( const uint8_t* pCompressed )
{
    HEADER* pHeader = (HEADER*)pCompressed;
    return sizeof(HEADER) + pHeader->compressedSize;
}
