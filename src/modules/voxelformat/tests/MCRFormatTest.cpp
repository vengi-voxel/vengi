/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxel/RawVolume.h"
#include "voxelformat/MCRFormat.h"
#include "voxelformat/QBFormat.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class MCRFormatTest: public AbstractVoxFormatTest {
};

TEST_F(MCRFormatTest, testLoad117) {
	scenegraph::SceneGraph sceneGraph;
	canLoad(sceneGraph, "r.0.-2.mca", 128);
	const scenegraph::SceneGraphNode &node = *sceneGraph.begin(scenegraph::SceneGraphNodeType::Model);
	ASSERT_EQ(node.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::RawVolume *v = node.volume();
	const int cnt = voxelutil::visitVolume(*v, [&](int, int, int, const voxel::Voxel &v) {}, voxelutil::VisitAll());
	EXPECT_EQ(32512, cnt);
}

TEST_F(MCRFormatTest, testLoad110) {
	scenegraph::SceneGraph sceneGraph;
	canLoad(sceneGraph, "minecraft_110.mca", 1024);
	const scenegraph::SceneGraphNode &node = *sceneGraph.begin(scenegraph::SceneGraphNodeType::Model);
	ASSERT_EQ(node.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::RawVolume *v = node.volume();
	const int cnt = voxelutil::visitVolume(*v, [&](int, int, int, const voxel::Voxel &v) {}, voxelutil::VisitAll());
	EXPECT_EQ(23296, cnt);
}

TEST_F(MCRFormatTest, testLoad113) {
	scenegraph::SceneGraph sceneGraph;
	canLoad(sceneGraph, "minecraft_113.mca", 1024);
	const scenegraph::SceneGraphNode &node = *sceneGraph.begin(scenegraph::SceneGraphNodeType::Model);
	ASSERT_EQ(node.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::RawVolume *v = node.volume();
	const int cnt = voxelutil::visitVolume(*v, [&](int, int, int, const voxel::Voxel &v) {}, voxelutil::VisitAll());
	EXPECT_EQ(17920, cnt);
}

}
