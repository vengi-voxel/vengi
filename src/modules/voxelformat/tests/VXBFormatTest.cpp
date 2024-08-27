/**
 * @file
 */

#include "voxelformat/private/sandbox/VXBFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class VXBFormatTest : public AbstractFormatTest {};

TEST_F(VXBFormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "sandbox-block2.vxb", 1);
}

TEST_F(VXBFormatTest, DISABLED_testLoadAndSave) {
	VXBFormat src;
	VXBFormat target;
	voxel::ValidateFlags flags = voxel::ValidateFlags::All;
	testLoadSaveAndLoadSceneGraph("sandbox-block2.vxb", src, "sandbox-block2-test.vxb", target, flags);
}

} // namespace voxelformat
