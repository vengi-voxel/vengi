/**
 * @file
 */

#include "voxelformat/private/slab6/SLABShared.h"
#include "AbstractFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class SLABSharedTest : public AbstractFormatTest {};

TEST_F(SLABSharedTest, testRGBColor) {
	io::BufferedReadWriteStream stream;
	const color::RGBA color1{0xFA, 0xDE, 0xDE};
	const color::RGBA color2{0xfe, 0xdf, 0xe1};
	ASSERT_TRUE(priv::writeRGBColor(stream, color1));
	ASSERT_TRUE(priv::writeRGBColor(stream, color2));
	stream.seek(0);
	color::RGBA rcolor1, rcolor2;
	ASSERT_TRUE(priv::readRGBColor(stream, rcolor1));
	ASSERT_TRUE(priv::readRGBColor(stream, rcolor2));
	EXPECT_EQ(rcolor1, color1);
	EXPECT_EQ(rcolor2, color2);
}

TEST_F(SLABSharedTest, testBGRColor) {
	io::BufferedReadWriteStream stream;
	const color::RGBA color1{0xFA, 0xDE, 0xDE};
	const color::RGBA color2{0xfe, 0xdf, 0xe1};
	ASSERT_TRUE(priv::writeBGRColor(stream, color1));
	ASSERT_TRUE(priv::writeBGRColor(stream, color2));
	stream.seek(0);
	color::RGBA rcolor1, rcolor2;
	ASSERT_TRUE(priv::readBGRColor(stream, rcolor1));
	ASSERT_TRUE(priv::readBGRColor(stream, rcolor2));
	EXPECT_EQ(rcolor1, color1);
	EXPECT_EQ(rcolor2, color2);
}

TEST_F(SLABSharedTest, testRGBScaledColor) {
	io::BufferedReadWriteStream stream;
	const color::RGBA color1{0xFA, 0xDE, 0xDE};
	const color::RGBA color2{0xfe, 0xdf, 0xe1};
	ASSERT_TRUE(priv::writeRGBScaledColor(stream, color1));
	ASSERT_TRUE(priv::writeRGBScaledColor(stream, color2));
	stream.seek(0);
	color::RGBA rcolor1, rcolor2;
	ASSERT_TRUE(priv::readRGBScaledColor(stream, rcolor1));
	ASSERT_TRUE(priv::readRGBScaledColor(stream, rcolor2));
	EXPECT_NEAR(rcolor1.r, color1.r, 4);
	EXPECT_NEAR(rcolor1.g, color1.g, 4);
	EXPECT_NEAR(rcolor1.b, color1.b, 4);
	EXPECT_NEAR(rcolor2.r, color2.r, 4);
	EXPECT_NEAR(rcolor2.g, color2.g, 4);
	EXPECT_NEAR(rcolor2.b, color2.b, 4);
}

} // namespace voxelformat
