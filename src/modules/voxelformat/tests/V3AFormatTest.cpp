/**
 * @file
 */

#include "voxelformat/private/voxel3d/V3AFormat.h"
#include "AbstractFormatTest.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class V3AFormatTest : public AbstractFormatTest {};

TEST_F(V3AFormatTest, testSaveSmallVoxel) {
	V3AFormat f;
	// TODO: color1[#000011ff], color2[#000000ff], delta[0.45600003004074097]
	voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Color);
	testSaveLoadVoxel("v3a-smallvolumesavetest.v3a", &f, -1, 1, flags, 0.004f);
}

TEST_F(V3AFormatTest, testSaveSmallVoxelV3B) {
	V3AFormat f;
	// TODO: color1[#000011ff], color2[#000000ff], delta[0.45600003004074097]
	voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Color);
	testSaveLoadVoxel("v3b-smallvolumesavetest.v3b", &f, -1, 1, flags, 0.004f);
}

} // namespace voxelformat
