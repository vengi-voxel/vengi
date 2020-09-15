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
	const io::FilePtr& file = open("cc.vxl");
	ASSERT_TRUE((bool)file) << "Could not open vxl file";
	RawVolume* loadedVolume = f.load(file);
	ASSERT_NE(nullptr, loadedVolume) << "Could not load vxl file";

	const io::FilePtr& fileSave = open("cc-save.vxl", io::FileMode::Write);
	EXPECT_TRUE(f.save(loadedVolume, fileSave));
	fileSave->close();
	ASSERT_TRUE(fileSave->open(io::FileMode::Read));
	RawVolume *savedVolume = f.load(fileSave);
	EXPECT_NE(nullptr, savedVolume) << "Could not load saved vxl file";
	if (savedVolume) {
		EXPECT_EQ(*savedVolume, *loadedVolume);
		delete savedVolume;
	}
	delete loadedVolume;
}

}
