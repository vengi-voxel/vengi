/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class StudioIOFormatTest : public AbstractFormatTest {};

TEST_F(StudioIOFormatTest, testLoadEncryptedIO) {
	// Use bundled parts/3001.dat - official library parts expand into large primitive trees.
	util::ScopedVarChange ldrawDir(cfg::VoxformatLDrawDir, "");
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "studioio-encrypted.io", 1, true);
	if (IsSkipped()) {
		return;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	const voxel::RawVolume *volume = node->volume();
	ASSERT_NE(volume, nullptr);
	EXPECT_GT(voxelutil::countVoxels(*volume), 0);
}

} // namespace voxelformat
