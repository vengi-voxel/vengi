/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "voxelformat/private/trove/TroveBlueprintFormat.h"

namespace voxelformat {

class TroveBlueprintFormatTest : public AbstractFormatTest {};

TEST_F(TroveBlueprintFormatTest, testSaveLoadVoxel) {
	TroveBlueprintFormat f;
	const voxel::ValidateFlags flags =
		voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Pivot);
	testSaveLoadVoxel("trove-smallvolumesavetest.blueprint", &f, 0, 1, flags);
}

} // namespace voxelformat
