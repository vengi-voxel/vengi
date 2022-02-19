/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXLFormat.h"

namespace voxel {

class VXLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXLFormatTest, testLoad) {
	VXLFormat f;
	std::unique_ptr<RawVolume> volume(load("cc.vxl", f));
	ASSERT_NE(nullptr, volume) << "Could not load vxl file";
}

TEST_F(VXLFormatTest, testLoadRGB) {
	VXLFormat f;
	std::unique_ptr<RawVolume> volume(load("rgb.vxl", f));
	ASSERT_NE(nullptr, volume) << "Could not load vxl file";
	testRGB(volume.get());
}

TEST_F(VXLFormatTest, testSave) {
	VXLFormat f;
	RawVolume* loadedVolume = load("cc.vxl", f);
	ASSERT_NE(nullptr, loadedVolume) << "Could not load vxl file";

	const io::FilePtr& file = open("cc-save.vxl", io::FileMode::Write);
	io::FileStream stream(file);
	ASSERT_TRUE(f.save(loadedVolume, file->name(), stream));
	f = VXLFormat();
	RawVolume *savedVolume = load("cc-save.vxl", f);
	EXPECT_NE(nullptr, savedVolume) << "Could not load saved vxl file " << file->name();
	if (savedVolume) {
		EXPECT_EQ(*savedVolume, *loadedVolume);
		delete savedVolume;
	}
	delete loadedVolume;
}

TEST_F(VXLFormatTest, testSaveSmallVoxel) {
	VXLFormat f;
	testSaveLoadVoxel("cc-smallvolumesavetest.vxl", &f);
}

}
