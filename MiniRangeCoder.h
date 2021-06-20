/**
 * @file MiniRangeCoder.h
 * 
 * Entropy encoding by RangeCoder for embedded systems.
 * Target data size is less than 255 bytes.
 * It assumes a CPU with a clock of tens of Mhz or less.
 * No division is used. The largest operation is multiplication of 16bit and 32bit.
 * The memory usage is 1KB for the table, the size of the input data, the size of the output data (max. input data + 2 bytes), and a small stack.
 * For table creation, the frequency of byte occurrences in the data must be known.
 * When should I use it? When you want to save bandwidth when transferring small packets.
 */

#ifndef ___MINI_RANGE_CODER_H___
#define ___MINI_RANGE_CODER_H___

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief A structure that defines the frequency of occurrence of a byte and its lower bound in RangeCoder.
 * @note Note that the total value of freq must be close to MAX_TOTAL_FREQ to be efficiently encoded.
 */
typedef struct
{
	uint16_t freq;		/*!< Frequency of occurrence of a byte, a number between 0 and MAX_TOTAL_FREQ. The total value of freq must be less than or equal to MAX_TOTAL_FREQ.  */
	uint16_t lower;		/*!< Lower used by RangeCoder. table.lower[0] = 0. table.lower[i] = table.lower[i-1] + table.freq[i-1]. */
} FREQ_LOWER;

/**
 * @brief Maximum total value of FREQ_LOWER.freq[i]
 */
#define MAX_TOTAL_FREQ  2048

/**
 * @brief Compress data with RangeCoder
 * @param[in] pSrc 		Source data.
 * @param[in] srcSize	Size of the source data.
 * @param[out] pOutCompressed Pointer to buffer to write the compressed data. A maximum of srcSize+2 bytes of data may be written.
 * @param[in] table		Frequency and lower tables. Define the probability of occurrence of each byte and the lower bound in RangeCoder.
 * @return Number of bytes written to pOutCompressed.
 * @attention If pSrc contains a byte with a frequency of 0, the data will not be compressed.
 * @note If the data is not compressed, it will be output at the original size plus 2 bytes.
 *
 * Perform entropy encoding with RangeCoder. 
 * The size after compression can be expected to be the size of entropy + 3~6 bytes.
 * Even if the data cannot be compressed, no more than the original size + 2 bytes will be output.
 */
uint16_t RangeCoderEncode(const uint8_t* pSrc, uint8_t srcSize, uint8_t* pOutCompressed, FREQ_LOWER table[0x100]);

/**
 * @brief Decompresses the data compressed by RangeCoderEncode.
 * 
 * @param[in] pCompressed 			Data compressed by RangeCoderEncode.
 * @param[out] pOutOriginal 		Pointer to the buffer to which original data will be output.
 * @param[out] pOutOriginalSize 	Pointer to output original size.
 * @param[in] table 				Frequency and lower tables. It must match table in RangeCoderEncode.
 * @return true 	Decode successful.
 * @return false 	Decoding failed. pCompressed or table is corrupted.
 * @attention If the data is not corrupted, it will always succeed. If the data is not corrupted, it will always succeed, but if the data is corrupted, it will not always fail.
 *
 * Decompresses entropy-encoded data by RangeCoderEncode.
 */
bool RangeCoderDecode(const uint8_t* pCompressed, uint8_t* pOutOriginal, uint8_t* pOutOriginalSize, FREQ_LOWER table[0x100]);

/**
 * @brief Get original data size
 * @param[in] pCompressed 	A sequence of bytes output by RangeCoderEncode.
 * @return Original data size 
 */
uint8_t GetOriginalSize( const uint8_t* pCompressed );

/**
 * @brief Get size of the data
 * @param[in] pCompressed 	A sequence of bytes output by RangeCoderEncode.
 * @return Data size. This is the same as the value returned by RangeCoderEncode.
 */
uint16_t GetDataSize( const uint8_t* pCompressed );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //___MINI_RANGE_CODER_H___
