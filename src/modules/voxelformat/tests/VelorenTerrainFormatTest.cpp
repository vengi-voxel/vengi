/**
 * @file
 */

#include "voxelformat/private/veloren/VelorenTerrainFormat.h"
#include "AbstractFormatTest.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class VelorenTerrainFormatTest : public AbstractFormatTest {};

TEST_F(VelorenTerrainFormatTest, testSaveSmallVolume) {
	VelorenTerrainFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::Transform | voxel::ValidateFlags::SceneGraphModels | voxel::ValidateFlags::Color;
	testSaveSmallVolume("testSaveSmallVolume.dat", &f, flags);
}

TEST_F(VelorenTerrainFormatTest, testSaveLoadVoxel) {
	VelorenTerrainFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::Transform | voxel::ValidateFlags::SceneGraphModels | voxel::ValidateFlags::Color;
	testSaveLoadVoxel("testSaveLoadVoxel.dat", &f, 0, 1, flags);
}

TEST_F(VelorenTerrainFormatTest, testChunk) {
	testLoad("chunkveloren.dat");
}

} // namespace voxelformat
