/**
 * @file
 */

#include "voxelformat/VolumeFormat.h"
#include "AbstractFormatTest.h"
#include "io/FilesystemArchive.h"

namespace voxelformat {

class VolumeFormatTest : public AbstractFormatTest {};

TEST_F(VolumeFormatTest, testImportPalette) {
	palette::Palette palette;
	EXPECT_EQ(0, palette.colorCount());
	EXPECT_TRUE(importPalette("vox_character.vox", palette));
	EXPECT_EQ(255, palette.colorCount());
}

TEST_F(VolumeFormatTest, testLoadFormat) {
	const char *files[] = {"rgb.csv", "rgb.cub", "rgb.gox", "rgb.qb", "rgb.qbcl",
						   "rgb.qef", "rgb.vox", "rgb.vxl", "rgb.vxm"};
	const io::ArchivePtr &archive = io::openFilesystemArchive(_testApp->filesystem());
	for (int i = 0; i < lengthof(files); ++i) {
		io::FileDescription fileDesc;
		fileDesc.set(files[i]);
		scenegraph::SceneGraph newSceneGraph;
		EXPECT_TRUE(loadFormat(fileDesc, archive, newSceneGraph, testLoadCtx)) << "Failed to load " << files[i];
		EXPECT_GT(newSceneGraph.size(), 0u) << "Empty scene graph for " << files[i];
	}
}

TEST_F(VolumeFormatTest, testIsMeshFormat) {
	EXPECT_TRUE(isMeshFormat("foo.obj", false));
	EXPECT_TRUE(isMeshFormat("foo.glb", false));
	EXPECT_TRUE(isMeshFormat("foo.gltf", false));
	EXPECT_TRUE(isMeshFormat("foo.stl", false));
}

} // namespace voxelformat
