/**
 * @file
 */

#include "voxelformat/private/mesh/TextureLookup.h"
#include "app/tests/AbstractTest.h"

namespace voxelformat {

class TextureLookupTest : public app::AbstractTest {};

TEST_F(TextureLookupTest, testLookupTexture) {
	EXPECT_NE(lookupTexture("glTF/cube/chr_knight.gox", "glTF/cube/Cube_BaseColor.png"), "");
	EXPECT_NE(lookupTexture("glTF/cube/chr_knight.gox", "Cube_BaseColor.png"), "");
	EXPECT_NE(lookupTexture("glTF/chr_knight.gox", "./cube/Cube_BaseColor.png"), "");
	EXPECT_NE(lookupTexture("glTF/chr_knight.gox", "cube/Cube_BaseColor.png"), "");
	// TODO: VOXELFORMAT: EXPECT_NE(lookupTexture("glTF/foo/bar/chr_knight.gox",
	// "../../cube/Cube_BaseColor.png"), "");
}

} // namespace voxelformat
