/**
 * @file
 */

#include "voxelformat/private/goxel/GoxFormat.h"
#include "AbstractFormatTest.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class GoxFormatTest : public AbstractFormatTest {};

TEST_F(GoxFormatTest, testLoad) {
	testLoad("test.gox");
}

TEST_F(GoxFormatTest, testSaveSmallVoxel) {
	GoxFormat f;
	testSaveLoadVoxel("goxel-smallvolumesavetest.gox", &f, -16, 15, voxel::ValidateFlags::None);
}

TEST_F(GoxFormatTest, testLoadRGB) {
	testRGB("rgb.gox");
}

TEST_F(GoxFormatTest, testLoadScreenshot) {
	testLoadScreenshot("chr_knight.gox", 128, 128, color::RGBA(158, 59, 59), 65, 27);
}

TEST_F(GoxFormatTest, testLoadShapeLayers) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "gox-shape-3-layers.gox", 4);
	int shapes = 0;
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const voxel::RawVolume *volume = node.volume();
		ASSERT_NE(nullptr, volume);
		if (voxelutil::countVoxels(*volume) > 0) {
			shapes++;
		}
	}
	EXPECT_EQ(3, shapes);
}

} // namespace voxelformat
