/**
 * @file
 */

#include "voxelformat/private/anivoxel/AniVoxelFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class AniVoxelFormatTest : public AbstractFormatTest {};

TEST_F(AniVoxelFormatTest, testLoadRobo) {
	testLoad("robo.voxa", 3);
}

TEST_F(AniVoxelFormatTest, testSaveLoad) {
	AniVoxelFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testSaveLoadVoxel("testsaveload.voxa", &f, 0, 10, flags);
}

} // namespace voxelformat
