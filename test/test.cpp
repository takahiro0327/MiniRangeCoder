#include <gtest/gtest.h>
#include "../MiniRangeCoder.h"
#include <algorithm>
#include <random>

#define HEADER_SIZE   2

void UnitTest(const uint8_t* pOriginal, uint32_t originalSize, FREQ_LOWER table[256])
{
  constexpr size_t margin = 4;
	uint8_t compressData[HEADER_SIZE + 255 + margin];
  memset(compressData,0xAA,sizeof(compressData));

	uint16_t compressed = MiniRangeCoderEncode(pOriginal, originalSize, compressData, table);

	ASSERT_LE(HEADER_SIZE, compressed);
  ASSERT_LE(compressed, originalSize + HEADER_SIZE);
  ASSERT_EQ(GetOriginalSize(compressData), originalSize);
  ASSERT_EQ(GetDataSize(compressData), compressed);
  ASSERT_TRUE( std::all_of(&compressData[compressed], &compressData[compressed+margin], [](uint8_t x){return x == 0xAA;}) );

	uint8_t decompressData[255 + margin];
  memset(decompressData,0x55,originalSize+margin);

	uint8_t decompressSize;
	bool res = MiniRangeCoderDecode(compressData, decompressData, &decompressSize, table);

	ASSERT_TRUE(res);
	ASSERT_EQ(decompressSize, originalSize);
  ASSERT_TRUE( std::all_of(&decompressData[originalSize], &decompressData[originalSize+margin], []( uint8_t x){return x == 0x55;}) );
	ASSERT_EQ(memcmp(decompressData, pOriginal, originalSize), 0);

  
  memset(compressData,0xAA,sizeof(compressData));
  uint32_t compressed32 = RangeCoderEncodeHeaderless(pOriginal,originalSize,compressData,table);

	ASSERT_LE(0, compressed32);
  ASSERT_LE(compressed32, originalSize);
  ASSERT_TRUE( std::all_of(&compressData[compressed32], &compressData[compressed32+margin], [](uint8_t x){return x == 0xAA;}) );

  memset(decompressData,0x55,originalSize+margin);
  res = RangeCoderDecodeHeaderless(compressData, compressed32, decompressData, originalSize, table);

  ASSERT_TRUE(res);
  ASSERT_TRUE( std::all_of(&decompressData[originalSize], &decompressData[originalSize+margin], []( uint8_t x){return x == 0x55;}) );
	ASSERT_EQ(memcmp(decompressData, pOriginal, originalSize), 0);
}

void SetLowerFromFreq(FREQ_LOWER table[256])
{
	table[0].lower = 0;

	for (int i = 1; i < 256; ++i)
	{
		table[i].lower = table[i - 1].lower + table[i - 1].freq;
	}

  ASSERT_LT(0, table[255].lower + table[255].freq);
	ASSERT_LE(table[255].lower + table[255].freq, MAX_TOTAL_FREQ);
}

TEST( MiniRangeCoderTest, OneByteTest)
{
	FREQ_LOWER table[256];
	memset(table, 0, sizeof(table));

	for (int byte = 0; byte < 0x100; ++byte)
	{
		uint8_t buff[255];
		memset(buff, byte, sizeof(buff));

    for( int freq = 1; freq <= MAX_TOTAL_FREQ; ++freq )
    {
      table[byte].freq = freq;
      SetLowerFromFreq(table);

      for (size_t size = 1; size <= sizeof(buff); ++size)
      {
        UnitTest(buff, size, table);
      }
    }

		table[byte].freq = 0;
	}
}

TEST( MiniRangeCoderTest, UniformTest)
{
	std::random_device seed;
	std::mt19937 engine(seed());

	std::uniform_int_distribution<> random(0, (int)MAX_TOTAL_FREQ);

	FREQ_LOWER table[256];
	memset(table, 0, sizeof(table));

	for (int i = 0; i < 256; ++i)
	{
		table[i].freq = MAX_TOTAL_FREQ / 256;
	}

	SetLowerFromFreq(table);

	uint8_t buff[255];

	for (int loop = 0; loop < (20<<10); ++loop)
	{
		for (size_t size = 1; size <= sizeof(buff); ++size)
		{
			for (size_t i = 0; i < size; ++i)
			{
				buff[i] = random(engine) & 0xFF;
			}

			UnitTest(buff, size, table);
		}
	}
}

TEST( MiniRangeCoderTest, RandomTest)
{
	std::random_device seed;
	std::mt19937 engine(seed());

	std::uniform_int_distribution<> random(0, (int)MAX_TOTAL_FREQ);

	uint8_t buff[255];
	uint8_t useBytes[0x100];

	for (int i = 0; i < 0x100; ++i)
		useBytes[i] = i;

	for (int loop0 = 0; loop0 < (50<<10); ++loop0)
	{
		std::shuffle(useBytes, useBytes + sizeof(useBytes), engine);

		FREQ_LOWER table[256] = { 0 };
		int nUsedByte = random(engine) % 255 + 1;

		int sum = 0;

		for (int i = 0; i < nUsedByte; ++i)
		{
			uint8_t byte = useBytes[i];

			int freq = random(engine);
			table[byte].freq = freq;

			sum += freq;
		}

		while ( sum < MAX_TOTAL_FREQ * 7 / 8 || MAX_TOTAL_FREQ < sum )
		{
			float r = (float)MAX_TOTAL_FREQ / sum * 0.99f;

			int newSum = 0;

			for (int i = 0; i < nUsedByte; ++i)
			{
				uint8_t byte = useBytes[i];

				int freq = table[byte].freq;
				freq = (int)std::floor(freq * r);
				if (freq <= 0) freq = 1;

				table[byte].freq = freq;
				newSum += freq;
			}

			sum = newSum;
		}

		SetLowerFromFreq(table);

		for (int size = 1; size <= 255; size += 15 )
		{
			for (int i = 0; i < size; ++i)
			{
				int freq = random(engine);

				uint8_t byte = useBytes[0];

				for (int j = 0; j < nUsedByte; ++j)
				{
					byte = useBytes[j];

					if (table[byte].lower <= freq && freq < table[byte].lower + table[byte].freq)
						break;
				}

				buff[i] = byte;
			}

			UnitTest(buff, size, table);
		}
	}
}

TEST( MiniRangeCoderTest, EdgeTest)
{
	FREQ_LOWER table[256];
	memset(table, 0, sizeof(table));

	for (int byte1 = 0; byte1 < 0x100; byte1 += 16)
	{
		for (int otherFreq = 1; otherFreq <= 3; ++otherFreq)
		{
			table[byte1].freq = MAX_TOTAL_FREQ - otherFreq;

			for (int byte2 = 0; byte2 < 0x100; byte2 += 16)
			{
				if (byte1 == byte2)
					continue;

				table[byte2].freq = otherFreq;

				SetLowerFromFreq(table);

				uint8_t buff[255];
				memset(buff, byte1, sizeof(buff));

				for (size_t size = 1; size < sizeof(buff); ++size)
				{
					buff[0] = byte2;
					UnitTest(buff, size, table);
					buff[0] = byte1;

					if (size >= 2)
					{
						buff[1] = byte2;
						UnitTest(buff, size, table);
						buff[1] = byte1;

						buff[size / 2] = byte2;
						UnitTest(buff, size, table);
						buff[size / 2] = byte1;

						buff[size - 2] = byte2;
						UnitTest(buff, size, table);
						buff[size - 2] = byte1;

						buff[size - 1] = byte2;
						UnitTest(buff, size, table);
						buff[size - 1] = byte1;
					}					
				}

				table[byte2].freq = 0;
			}

			table[byte1].freq = 0;
		}
	}
}

TEST( MiniRangeCoderTest, TwoByteTest)
{
	FREQ_LOWER table[256];
	memset(table, 0, sizeof(table));

	uint8_t buff[255];

	std::random_device seed;
	std::mt19937 engine(seed());
	std::uniform_int_distribution<> random(0, (int)MAX_TOTAL_FREQ);

	for (int byte1 = 0; byte1 < 0x100; byte1 += 64)
	{
		for (int byte2 = 1; byte2 < 0x100; byte2 += 64)
		{
			for (int byte1Freq = 1; byte1Freq <= MAX_TOTAL_FREQ; ++byte1Freq )
			{
				int byte2Freq = MAX_TOTAL_FREQ - byte1Freq;

				table[byte1].freq = byte1Freq;
				table[byte2].freq = byte2Freq;
				SetLowerFromFreq(table);

				for (size_t i = 0; i < sizeof(buff); ++i)
					buff[i] = random(engine) <= byte1Freq ? byte1 : byte2;

				for (int size = 1; size < 0x100; ++size)
				{
					UnitTest(buff, size, table);
				}
			}

			table[byte1].freq = 0;
			table[byte2].freq = 0;
		}
	}
}
