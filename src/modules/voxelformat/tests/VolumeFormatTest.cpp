/**
 * @file
 */

#include "voxelformat/VolumeFormat.h"
#include "AbstractVoxFormatTest.h"
#include "io/File.h"
#include "io/FileStream.h"

namespace voxelformat {

class VolumeFormatTest : public AbstractVoxFormatTest {};

TEST_F(VolumeFormatTest, testImportPalette) {
	voxel::Palette palette;
	EXPECT_EQ(0, palette.colorCount());
	EXPECT_TRUE(importPalette("vox_character.vox", palette));
	EXPECT_EQ(255, palette.colorCount());
}

TEST_F(VolumeFormatTest, testLoadFormat) {
	const char *files[] = {"rgb.csv", "rgb.cub", "rgb.gox", "rgb.qb", "rgb.qbcl",
						   "rgb.qef", "rgb.vox", "rgb.vxl", "rgb.vxm"};
	for (int i = 0; i < lengthof(files); ++i) {
		io::FilePtr file = open(files[i]);
		ASSERT_TRUE(file->validHandle());
		io::FileStream stream(file);
		io::FileDescription fileDesc;
		fileDesc.set(file->name());
		scenegraph::SceneGraph newSceneGraph;
		EXPECT_TRUE(loadFormat(fileDesc, stream, newSceneGraph, testLoadCtx)) << "Failed to load " << files[i];
		EXPECT_GT(newSceneGraph.size(), 0u) << "Empty scene graph for " << files[i];
	}
}

TEST_F(VolumeFormatTest, testIsMeshFormat) {
	EXPECT_TRUE(isMeshFormat("foo.obj", false));
	EXPECT_TRUE(isMeshFormat("foo.glb", false));
	EXPECT_TRUE(isMeshFormat("foo.gltf", false));
	EXPECT_TRUE(isMeshFormat("foo.ply", false));
	EXPECT_TRUE(isMeshFormat("foo.stl", false));
}

} // namespace voxelformat
