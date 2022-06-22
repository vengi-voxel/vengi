/**
 * @file
 */

#include "voxelformat/VolumeFormat.h"
#include "AbstractVoxFormatTest.h"
#include "app/App.h"
#include "io/File.h"

namespace voxelformat {

class VolumeFormatTest : public AbstractVoxFormatTest {};

TEST_F(VolumeFormatTest, testImportPalette) {
	voxel::Palette palette;
	EXPECT_EQ(0, palette.colorCount);
	EXPECT_TRUE(importPalette("vox_character.vox", palette));
	EXPECT_EQ(256, palette.colorCount);
}

TEST_F(VolumeFormatTest, testLoadFormat) {
	io::FilePtr file = io::filesystem()->open("vox_character.vox");
	ASSERT_TRUE(file->validHandle());
	io::FileStream stream(file);
	SceneGraph newSceneGraph;
	EXPECT_TRUE(loadFormat(file->name(), stream, newSceneGraph));
	EXPECT_EQ(16u, newSceneGraph.size());
}

TEST_F(VolumeFormatTest, testIsMeshFormat) {
	EXPECT_TRUE(isMeshFormat("foo.obj"));
	EXPECT_TRUE(isMeshFormat("foo.glb"));
	EXPECT_TRUE(isMeshFormat("foo.gltf"));
	EXPECT_TRUE(isMeshFormat("foo.ply"));
	EXPECT_TRUE(isMeshFormat("foo.stl"));
}

} // namespace voxelformat
