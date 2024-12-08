/**
 * @file
 */

#include "voxelformat/private/mesh/TextureLookup.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "io/FilesystemArchive.h"

namespace voxelformat {

class TextureLookupTest : public app::AbstractTest {};

TEST_F(TextureLookupTest, testLookupTexture) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(io::filesystem());
	EXPECT_NE(lookupTexture("glTF/cube/chr_knight.gox", "glTF/cube/Cube_BaseColor.png", archive), "");
	EXPECT_NE(lookupTexture("glTF/cube/chr_knight.gox", "Cube_BaseColor.png", archive), "");
	EXPECT_NE(lookupTexture("glTF/chr_knight.gox", "./cube/Cube_BaseColor.png", archive), "");
	EXPECT_NE(lookupTexture("glTF/chr_knight.gox", "cube/Cube_BaseColor.png", archive), "");
	// TODO: VOXELFORMAT: EXPECT_NE(lookupTexture("glTF/foo/bar/chr_knight.gox",
	// "../../cube/Cube_BaseColor.png", archive), "");
}

} // namespace voxelformat
