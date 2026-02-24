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

protected:
	io::ArchivePtr _archive;
	const core::String _input{"glTF/cube/Cube.gltf"};
	const core::String _expected{"glTF/cube/Cube_BaseColor.png"};

public:
	bool onInitApp() override {
		Super::onInitApp();
		_archive = io::openFilesystemArchive(io::filesystem());
		core::Var::registerVar(core::VarDef(cfg::VoxformatTexturePath, ""));
		return true;
	}
};

TEST_F(TextureLookupTest, testLookupTextureWorkingDirectory) {
	EXPECT_EQ(lookupTexture(_input, "Cube_BaseColor.png", _archive), _expected);
	EXPECT_EQ(lookupTexture(_input, "./Cube_BaseColor.png", _archive), _expected);
}

TEST_F(TextureLookupTest, testLookupTexturePartialMatch) {
	EXPECT_EQ(lookupTexture(_input, "cube/Cube_BaseColor.png", _archive), _expected);
	EXPECT_EQ(lookupTexture(_input, "./cube/Cube_BaseColor.png", _archive), _expected);
}

TEST_F(TextureLookupTest, testLookupTextureRelativePath) {
	EXPECT_EQ(lookupTexture(_input, "../../cube/Cube_BaseColor.png", _archive), _expected);
}

TEST_F(TextureLookupTest, testLookupTextureNonExistingAbsolutePath) {
	EXPECT_EQ(lookupTexture(_input, "/non-existing/cube/Cube_BaseColor.png", _archive), _expected);
}

} // namespace voxelformat
