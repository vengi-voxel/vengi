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

	ASSERT_TRUE(zip::compress(inputBuf, inputBufSize, outputBuf, outputBufSize)) << "Failed to compress buffer";
}

TEST_F(ZipTest, testUncompress) {
	constexpr size_t inputBufSize = 64;
	const uint8_t inputBuf[inputBufSize] { 0 };
	constexpr size_t outputBufSize = 64;
	uint8_t outputBuf[outputBufSize];
	size_t finalSize = 0;

	ASSERT_TRUE(zip::compress(inputBuf, inputBufSize, outputBuf, outputBufSize, &finalSize)) << "Failed to compress buffer";
	ASSERT_LT(finalSize, inputBufSize) << "No compression - expected the compressed size to be smaller than the input size";

	constexpr size_t outputAfterCompressBufSize = inputBufSize;
	uint8_t outputAfterCompressBuf[outputAfterCompressBufSize] { 0 };
	ASSERT_TRUE(zip::uncompress(outputBuf, outputBufSize, outputAfterCompressBuf, outputAfterCompressBufSize)) << "Failed to compress buffer";

	for (size_t i = 0; i < outputAfterCompressBufSize; ++i) {
		ASSERT_EQ(inputBuf[i], outputAfterCompressBuf[i]);
	}
}

}
