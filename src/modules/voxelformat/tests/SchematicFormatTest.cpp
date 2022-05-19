/**
 * @file
 */

#include "voxelformat/SchematicFormat.h"
#include "AbstractVoxFormatTest.h"
#include "io/FileStream.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelformat {

class SchematicFormatTest : public AbstractVoxFormatTest {};

TEST_F(SchematicFormatTest, DISABLED_testLoad) {
	// https://www.planetminecraft.com/project/viking-island-4911284/
	canLoad("viking_island.schematic");
}

TEST_F(SchematicFormatTest, DISABLED_testSaveSmallVoxel) {
	SchematicFormat f;
	testSaveLoadVoxel("minecraft-smallvolumesavetest.schematic", &f);
}

} // namespace voxelformat
