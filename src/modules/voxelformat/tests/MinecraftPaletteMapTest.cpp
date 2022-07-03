/**
 * @file
 */

#include "voxelformat/private/MinecraftPaletteMap.h"
#include "app/tests/AbstractTest.h"

namespace voxelformat {

class MinecraftPaletteMapTest : public app::AbstractTest {};

TEST_F(MinecraftPaletteMapTest, testParse) {
	EXPECT_EQ(164,
			  findPaletteIndex(
				  "minecraft:dark_oak_stairs[facing=east,half=bottom,shape=outer_left,waterlogged=false][INT] = 554"));
}

} // namespace voxelformat
