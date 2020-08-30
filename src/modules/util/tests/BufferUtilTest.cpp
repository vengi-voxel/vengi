/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "util/BufferUtil.h"

namespace util {

class BufferUtilTest : public core::AbstractTest {
};

TEST_F(BufferUtilTest, testIndexCompress) {
	const uint32_t indices[] = { 1, 2, 3, 4 };
	size_t bytesPerIndex;
	uint8_t out[4];
	indexCompress(indices, sizeof(indices), bytesPerIndex, out, sizeof(out));

	EXPECT_EQ(1U, bytesPerIndex);

	EXPECT_EQ(1U, out[0]);
	EXPECT_EQ(2U, out[1]);
	EXPECT_EQ(3U, out[2]);
	EXPECT_EQ(4U, out[3]);
}

TEST_F(BufferUtilTest, testIndexNoCompress) {
	const uint8_t indices[] = { 1, 2, 3, 4 };
	size_t bytesPerIndex;
	uint8_t out[4];
	indexCompress(indices, sizeof(indices), bytesPerIndex, out, sizeof(out));

	EXPECT_EQ(1U, bytesPerIndex);

	EXPECT_EQ(1U, out[0]);
	EXPECT_EQ(2U, out[1]);
	EXPECT_EQ(3U, out[2]);
	EXPECT_EQ(4U, out[3]);
}

}
