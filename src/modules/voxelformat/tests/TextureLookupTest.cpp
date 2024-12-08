/**
 * @file
 */

#include "voxelformat/private/mesh/TextureLookup.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "io/FilesystemArchive.h"

namespace voxelformat {

class TextureLookupTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

public:
	bool onInitApp() override {
		Super::onInitApp();
		core::Var::get(cfg::VoxformatTexturePath, "");
		return true;
	}
};

TEST_F(TextureLookupTest, testLookupTexture) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(io::filesystem());

	EXPECT_EQ(lookupTexture("glTF/cube/Cube.gltf", "Cube_BaseColor.png", archive), "glTF/cube/Cube_BaseColor.png")
		<< "Failed to find the texture in the directory of the model";
	EXPECT_EQ(lookupTexture("glTF/cube/Cube.gltf", "./Cube_BaseColor.png", archive), "glTF/cube/./Cube_BaseColor.png")
		<< "Failed to search the current directory";

	EXPECT_NE(lookupTexture("glTF/cube/chr_knight.gox", "Cube_BaseColor.png", archive), "");
	EXPECT_NE(lookupTexture("glTF/chr_knight.gox", "./cube/Cube_BaseColor.png", archive), "");
	EXPECT_NE(lookupTexture("glTF/chr_knight.gox", "cube/Cube_BaseColor.png", archive), "");
	// TODO: VOXELFORMAT: EXPECT_NE(lookupTexture("glTF/foo/bar/chr_knight.gox",
	// "../../cube/Cube_BaseColor.png", archive), "");
}

} // namespace voxelformat
