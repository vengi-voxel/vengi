/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/CubFormat.h"

namespace voxel {

class CubFormatTest: public AbstractVoxFormatTest {
};

TEST_F(CubFormatTest, testLoad) {
	CubFormat f;
	std::unique_ptr<RawVolume> volume(load("cw.cub", f));
	ASSERT_NE(nullptr, volume) << "Could not load volume";
}

TEST_F(CubFormatTest, testLoadPalette) {
	CubFormat f;
	voxel::Palette pal;
	EXPECT_EQ(5, loadPalette("rgb.cub", f, pal));
}

TEST_F(CubFormatTest, testLoadRGB) {
	CubFormat f;
	std::unique_ptr<RawVolume> volume(load("rgb.cub", f));
	ASSERT_NE(nullptr, volume) << "Could not load volume";
	testRGB(volume.get());
}

TEST_F(CubFormatTest, testSaveSmallVoxel) {
	CubFormat f;
	testSaveLoadVoxel("cw-smallvolumesavetest.cub", &f);
}

}
