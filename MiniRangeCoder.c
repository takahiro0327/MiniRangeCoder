#include "MiniRangeCoder.h"
#include <assert.h>
#include <string.h>

#define RANGE_SHIFT		11
#define FIRST_RANGE		0xFFFFFFFF

static_assert((1 << RANGE_SHIFT) == MAX_TOTAL_FREQ, "failed MAX_TOTAL_FREQ or RANGE_SHIFT");

uint32_t _RangeCoderEncode(const uint8_t* pSrc, uint32_t srcSize, uint8_t* pCompressed, FREQ_LOWER table[0x100])
{
	uint32_t range = FIRST_RANGE;
	uint32_t lower = 0;

    const uint8_t* pSrcFirst = pSrc;
	const uint8_t* pSrcLast = pSrc + srcSize;

    uint8_t* pFirstCompressed = pCompressed;
	uint8_t* pLimitCompressed = pCompressed + srcSize;
    
	if (srcSize <= 0)
		goto not_compress;
    
    goto start;

	while( pSrc != pSrcLast )
	{
		while (range < 0x01000000)
		{
			*(pCompressed++) = (uint8_t)(lower >> (sizeof(lower)*8-8));
			range <<= 8;
			lower <<= 8;

			if (pLimitCompressed == pCompressed)
				goto not_compress;
		}

start:;

		uint8_t byte = *(pSrc++);

        uint16_t fre = table[byte].freq;
        uint16_t low = table[byte].lower;

		uint32_t r = range >> RANGE_SHIFT;
		uint32_t l = r * low;

		if (fre == 0)
			goto not_compress;	//Detect bytes with a frequency of 0

		range  = r * fre;

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

	while( lower )
	{
		*(pCompressed++) = (uint8_t)(lower >> (sizeof(lower)*8-8));
		lower <<= 8;

		if (pLimitCompressed == pCompressed)
			goto not_compress;
	}

	return (uint32_t)((uintptr_t)pCompressed - (uintptr_t)pFirstCompressed);

not_compress:

	//Copy the original data since it could not be compressed.
	memcpy(pFirstCompressed, pSrcFirst, srcSize);
	return srcSize;
}

bool _RangeCoderDecode(const uint8_t* pCompressed, uint32_t compressedSize, uint8_t* pOriginal, uint32_t originalSize, FREQ_LOWER table[0x100])
{
	uint32_t range = FIRST_RANGE;
	uint32_t lower = 0;

	if (compressedSize > originalSize)
        goto corrupted;		

	if (compressedSize < originalSize)
	{
		const uint8_t* pCompressedLast = pCompressed + compressedSize;

		for (int i = 0; i < 4; ++i)
		{
			lower <<= 8;
			if (pCompressed != pCompressedLast)
				lower += *(pCompressed++);
		}

		uint8_t* pOriginalLast = pOriginal + originalSize;
		
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
	memcpy(pOriginal, pCompressed, originalSize);
	return true;

corrupted:

	return false;
}

typedef struct
{
	uint8_t originalSize;
	uint8_t compressedSize;
} HEADER;

uint16_t MiniRangeCoderEncode(const uint8_t* pSrc, uint8_t srcSize, uint8_t* pCompressed, FREQ_LOWER table[0x100])
{
    HEADER* header = (HEADER*)pCompressed;
    header->originalSize = srcSize;
    header->compressedSize = _RangeCoderEncode(pSrc,srcSize,pCompressed + sizeof(HEADER),table);
    return sizeof(HEADER) + header->compressedSize;
}

bool MiniRangeCoderDecode(const uint8_t* pCompressed, uint8_t* pOutOriginal, uint8_t* pOutOriginalSize, FREQ_LOWER table[0x100])
{
    HEADER* header = (HEADER*)pCompressed;
    *pOutOriginalSize = header->originalSize;
    return _RangeCoderDecode(pCompressed + sizeof(HEADER), header->compressedSize, pOutOriginal, header->originalSize, table);
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

uint32_t RangeCoderEncodeHeaderless( const uint8_t* pSrc, uint32_t srcSize, uint8_t* pCompressed, FREQ_LOWER table[0x100] )
{
    return _RangeCoderEncode(pSrc,srcSize,pCompressed,table);
}

bool RangeCoderDecodeHeaderless(const uint8_t* pCompressed, uint32_t compressedSize, uint8_t* pOutOriginal, uint32_t originalSize, FREQ_LOWER table[0x100])
{
    return _RangeCoderDecode(pCompressed, compressedSize, pOutOriginal, originalSize, table);
}
