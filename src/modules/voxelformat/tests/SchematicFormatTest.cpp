/**
 * @file
 */

#include "voxelformat/private/minecraft/SchematicFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class SchematicFormatTest : public AbstractFormatTest {};

TEST_F(SchematicFormatTest, testLoadLitematic) {
	testLoad("test.litematic");
}

TEST_F(SchematicFormatTest, testLoadVikingIsland) {
	// https://www.planetminecraft.com/project/viking-island-4911284/
	testLoad("viking_island.schematic");
}

TEST_F(SchematicFormatTest, testLoadStructory) {
	// https://www.planetminecraft.com/data-pack/structory/
	testLoad("brick_chimney_1.nbt");
}

TEST_F(SchematicFormatTest, DISABLED_testSaveSmallVoxel) {
	SchematicFormat f;
	testSaveLoadVoxel("minecraft-smallvolumesavetest.schematic", &f);
}

} // namespace voxelformat
