/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXLFormat.h"

namespace voxel {

class VXLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXLFormatTest, testLoad) {
	const io::FilePtr& file = open("cc.vxl");
	ASSERT_TRUE((bool)file) << "Could not open vxl file";
	VXLFormat f;
	RawVolume* volume = f.load(file);
	ASSERT_NE(nullptr, volume) << "Could not load vxl file";
	delete volume;
}

TEST_F(VXLFormatTest, DISABLED_testLoadRGB) {
	const io::FilePtr& file = open("rgb.vxl");
	ASSERT_TRUE((bool)file) << "Could not open vxl file";
	VXLFormat f;
	RawVolume* volume = f.load(file);
	ASSERT_NE(nullptr, volume) << "Could not load vox file";
	testRGB(volume);
	delete volume;
}

TEST_F(VXLFormatTest, DISABLED_testSave) {
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
