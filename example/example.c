#include "../MiniRangeCoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main()
{
    //Create a table. It is recommended to bake this into the source code in advance.
    static FREQ_LOWER table[256];
    {
        //Each 0 or 1 has a 50% chance of occurring.
        table[0].freq = MAX_TOTAL_FREQ / 2;
        table[1].freq = MAX_TOTAL_FREQ / 2;

        for( int i = 1; i < 256; ++i )
            table[i].lower = table[i-1].lower + table[i-1].freq;

        assert( table[255].lower + table[255].freq <= MAX_TOTAL_FREQ );
    }

    //Generate random data of 0 or 1
    uint8_t data[255];
    {
        for( size_t i = 0; i < sizeof(data); ++i )
            data[i] = rand() & 1;   // 0 or 1
    }

    //Compression.  
    uint8_t compress[257]; // Maximum output size is the input size + 2.
    RangeCoderEncode( data, sizeof(data), compress, table );
    uint16_t compressSize = GetDataSize(compress);

    //In this case it is 14%. 14%=(255/8+4) / 255. +4 bytes is the header (2 bytes) and overhead.
    printf("Original size : %d\n", (int)sizeof(data));
    printf("Compress size : %d(%.1f%%)\n", compressSize, 100.0f * compressSize / sizeof(data) );

    int originalSize = GetOriginalSize(compress);
    uint8_t* pDecompress = malloc(originalSize);

    //Decompression.
    uint8_t decompressSize;
    RangeCoderDecode(compress, pDecompress, &decompressSize, table);
    
    printf("%s\n", 
        originalSize == sizeof(data) &&
        decompressSize == sizeof(data) &&
        memcmp(pDecompress,data,sizeof(data)) == 0 ? "OK" : "NG!!!"
    );

    free(pDecompress);

    return 0;
}
