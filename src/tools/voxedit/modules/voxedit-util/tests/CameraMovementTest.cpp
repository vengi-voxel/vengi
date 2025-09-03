/**
 * @file
 */

#include "../CameraMovement.h"
#include "app/tests/AbstractTest.h"
#include "command/CommandHandler.h"
#include "command/tests/TestHelper.h"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/Clipper.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/Movement.h"
#include "util/VarUtil.h"
#include "video/Camera.h"
#include "voxedit-util/Config.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "gtest/gtest.h"

namespace voxedit {

class CameraMovementTest : public app::AbstractTest {
protected:
	class CameraMovementExt : public CameraMovement {
	public:
		scenegraph::Clipper &clipper() {
			return _clipper;
		}

		util::Movement &movement() {
			return _movement;
		}
	};

	void prepareSceneGraph(scenegraph::SceneGraph &sceneGraph) {
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("model");
		voxel::Region region(0, 15);
		core_assert(region.isValid());
		voxel::RawVolume *v = new voxel::RawVolume(region);
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		// fill the ground floor with a solid voxel to walk on
		for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				v->setVoxel(x, 0, z, voxel);
			}
		}
		node.setVolume(v, true);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node)));
	}
};

TEST_F(CameraMovementTest, test) {
	CameraMovementExt m;
	m.construct();
	ASSERT_TRUE(m.init());

	scenegraph::SceneGraph sceneGraph;
	prepareSceneGraph(sceneGraph);
	video::Camera camera;
	camera.setRotationType(video::CameraRotationType::Eye);
	camera.setSize({800, 600});
	const glm::vec3 worldPos{0.0f, 10.0f, 0.0f};
	camera.setWorldPosition(worldPos);
	camera.update(0.0);
	const float eyeY = camera.eye().y;
	EXPECT_FLOAT_EQ(eyeY, worldPos.y);

	{
		command::ScopedButtonCommand move("move_left", 10, 0.0);
		const scenegraph::FrameIndex frameIdx = 0;
		m.update(0.0001, &camera, sceneGraph, frameIdx);
		EXPECT_TRUE(camera.dirty());
		camera.update(0.0);
	}
	m.shutdown();
}

} // namespace voxedit
