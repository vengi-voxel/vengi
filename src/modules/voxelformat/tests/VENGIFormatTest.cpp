/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/private/vengi/VENGIFormat.h"

namespace voxelformat {

class VENGIFormatTest : public AbstractVoxFormatTest {};

TEST_F(VENGIFormatTest, testSaveSmallVolume) {
	VENGIFormat f;
	testSaveSmallVolume("testSaveSmallVolume.vengi", &f);
}

TEST_F(VENGIFormatTest, testSaveLoadVoxel) {
	VENGIFormat f;
	testSaveLoadVoxel("testSaveLoadVoxel.vengi", &f);
}

} // namespace voxelformat
