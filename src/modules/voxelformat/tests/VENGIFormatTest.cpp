/**
 * @file
 */

#include "voxelformat/private/vengi/VENGIFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class VENGIFormatTest : public AbstractFormatTest {};

TEST_F(VENGIFormatTest, testSaveSmallVolume) {
	VENGIFormat f;
	testSaveSmallVolume("testSaveSmallVolume.vengi", &f);
}

TEST_F(VENGIFormatTest, testSaveLoadVoxel) {
	VENGIFormat f;
	testSaveLoadVoxel("testSaveLoadVoxel.vengi", &f);
}

} // namespace voxelformat
