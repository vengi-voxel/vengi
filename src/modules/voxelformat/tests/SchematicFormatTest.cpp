/**
 * @file
 */

#include "voxelformat/SchematicFormat.h"
#include "AbstractVoxFormatTest.h"
#include "io/FileStream.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelformat {

class SchematicFormatTest : public AbstractVoxFormatTest {};

TEST_F(SchematicFormatTest, DISABLED_testLoadVikingIsland) {
	// https://www.planetminecraft.com/project/viking-island-4911284/
	canLoad("viking_island.schematic");
}

TEST_F(SchematicFormatTest, DISABLED_testLoadStructory) {
	// https://www.planetminecraft.com/data-pack/structory/
	canLoad("brick_chimney_1.nbt");
}

TEST_F(SchematicFormatTest, DISABLED_testSaveSmallVoxel) {
	SchematicFormat f;
	testSaveLoadVoxel("minecraft-smallvolumesavetest.schematic", &f);
}

} // namespace voxelformat
