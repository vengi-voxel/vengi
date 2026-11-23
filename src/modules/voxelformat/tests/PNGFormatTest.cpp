/**
 * @file
 */

#include "voxelformat/private/image/PNGFormat.h"
#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "math/Vec3i.h"

namespace voxelformat {

class PNGFormatTest : public AbstractFormatTest {};

TEST_F(PNGFormatTest, testLoadPlane) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "fuel_can.png", 1);
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_TRUE(node != nullptr);
	const voxel::Region &region = node->region();
	EXPECT_EQ(region.getDimensionsInVoxels(), math::Vec3i(128, 128, 1));
}

TEST_F(PNGFormatTest, testLoadVolume) {
	util::ScopedVarChange scoped(cfg::VoxformatImageImportType, PNGFormat::ImageType::Volume);
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test-heightmap.png", 1);
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_TRUE(node != nullptr);
	const voxel::Region &region = node->region();
	EXPECT_EQ(region.getDimensionsInVoxels(), math::Vec3i(8, 8, 3));
}

TEST_F(PNGFormatTest, testLoadHeightmap) {
	util::ScopedVarChange scoped(cfg::VoxformatImageImportType, PNGFormat::ImageType::Heightmap);
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test-heightmap.png", 1);
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_TRUE(node != nullptr);
	const voxel::Region &region = node->region();
	EXPECT_EQ(region.getDimensionsInVoxels(), math::Vec3i(8, 255, 8));
}

} // namespace voxelformat
