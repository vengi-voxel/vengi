/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class MCRFormatTest : public AbstractFormatTest {};

TEST_F(MCRFormatTest, testLoad117) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "r.0.-2.mca", 128);
	const scenegraph::SceneGraphNode &node = *sceneGraph.begin(scenegraph::SceneGraphNodeType::Model);
	ASSERT_EQ(node.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::RawVolume *v = node.volume();
	const voxel::Voxel vxls[] = {
		v->voxel(0, 62, -576),
		v->voxel(0, -45, -576),
		v->voxel(0, -45, -566),
		v->voxel(0, -62, -576),
		v->voxel(0, -64, -576)
	};
	const uint8_t clrs[] = {
		22,
		22,
		6,
		118,
		7
	};
	for (int i = 0; i < 5; ++i) {
		const voxel::Voxel expected = voxel::createVoxel(voxel::VoxelType::Generic, clrs[i]);
		EXPECT_TRUE(vxls[i].isSame(expected)) << vxls[i];
	}
	EXPECT_EQ(32512, v->region().voxels());
}

TEST_F(MCRFormatTest, testLoad110) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "minecraft_110.mca", 1024);
	const scenegraph::SceneGraphNode &node = *sceneGraph.begin(scenegraph::SceneGraphNodeType::Model);
	ASSERT_EQ(node.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::RawVolume *v = node.volume();
	EXPECT_EQ(23296, v->region().voxels());
}

TEST_F(MCRFormatTest, testLoad113) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "minecraft_113.mca", 1024);
	const scenegraph::SceneGraphNode &node = *sceneGraph.begin(scenegraph::SceneGraphNodeType::Model);
	ASSERT_EQ(node.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::RawVolume *v = node.volume();
	EXPECT_EQ(17920, v->region().voxels());
}

} // namespace voxelformat
