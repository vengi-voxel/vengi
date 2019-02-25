/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/Zip.h"

namespace core {

class ZipTest: public AbstractTest {
};

TEST_F(ZipTest, testCompress) {
	constexpr size_t inputBufSize = 64;
	const uint8_t inputBuf[inputBufSize] { 0 };
	constexpr size_t outputBufSize = 64;
	uint8_t outputBuf[outputBufSize];

	size_t finalSize = 0;
	EXPECT_TRUE(zip::compress(inputBuf, inputBufSize, outputBuf, outputBufSize, &finalSize)) << "Failed to compress buffer";
	EXPECT_LT(finalSize, inputBufSize) << "No compression - expected the compressed size to be smaller than the input size";
}

TEST_F(ZipTest, testUncompress) {
	constexpr size_t inputBufSize = 64;
	uint8_t inputBuf[inputBufSize] {0};
	for (size_t i = 0u; i < inputBufSize / 2; i += 2) {
		inputBuf[i + 0] = (uint8_t)i + 1;
		inputBuf[i + 1] = (uint8_t)i + 1;
	}
	constexpr size_t outputBufSize = 64;
	uint8_t outputBuf[outputBufSize];
	size_t finalSize = 0;

	EXPECT_TRUE(zip::compress(inputBuf, inputBufSize, outputBuf, outputBufSize, &finalSize)) << "Failed to compress buffer";
	EXPECT_LT(finalSize, inputBufSize) << "No compression - expected the compressed size to be smaller than the input size";

	constexpr size_t outputAfterCompressBufSize = inputBufSize;
	uint8_t outputAfterCompressBuf[outputAfterCompressBufSize] { 0 };
	EXPECT_TRUE(zip::uncompress(outputBuf, outputBufSize, outputAfterCompressBuf, outputAfterCompressBufSize)) << "Failed to compress buffer";

	for (size_t i = 0; i < outputAfterCompressBufSize; ++i) {
		EXPECT_EQ(inputBuf[i], outputAfterCompressBuf[i]);
	}
}

}
