/**
 * @file
 */

#include "voxelformat/private/benvoxel/BenVoxelFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class BenVoxelFormatTest : public AbstractFormatTest {};

TEST_F(BenVoxelFormatTest, DISABLED_testLoadJSON) {
	testLoad("sora.ben.json");
}

TEST_F(BenVoxelFormatTest, DISABLED_testLoadBinary) {
	testLoad("sora.ben");
}

TEST_F(BenVoxelFormatTest, DISABLED_testSaveSmallVolumeBinary) {
	BenVoxelFormat f;
	testSaveSmallVolume("testSaveSmallVolume.ben", &f);
}

TEST_F(BenVoxelFormatTest, DISABLED_testSaveSmallVolumeJSON) {
	BenVoxelFormat f;
	testSaveSmallVolume("testSaveSmallVolume.ben.json", &f);
}

} // namespace voxelformat
