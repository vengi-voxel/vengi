/**
 * @file
 */

#include "voxelformat/SceneGraphUtil.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxelformat {

class SceneGraphUtilTest : public app::AbstractTest {};

TEST_F(SceneGraphUtilTest, testAddSceneGraphNodes) {
	SceneGraph source;
	int groupNodeId = -1;
	{
		SceneGraphNode node(SceneGraphNodeType::Group);
		node.setName("group");
		groupNodeId = source.emplace(core::move(node));
	}
	{
		SceneGraphNode node;
		node.setName("model");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		source.emplace(core::move(node), groupNodeId);
	}
	SceneGraph target;
	EXPECT_EQ(1, addSceneGraphNodes(target, source, target.root().id()));
}

} // namespace voxelformat
