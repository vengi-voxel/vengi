/**
 * @file
 */

#include "../CameraMovement.h"
#include "app/tests/AbstractTest.h"
#include "command/tests/TestHelper.h"
#include "core/Var.h"
#include "core/ConfigVar.h"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/Movement.h"
#include "util/VarUtil.h"
#include "video/Camera.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "gtest/gtest.h"

namespace voxelrender {

class CameraMovementTest : public app::AbstractTest {
protected:
	class CameraMovementExt : public CameraMovement {
	public:
		util::Movement &movement() {
			return _movement;
		}
	};

	bool isInsideSolid(const glm::vec3 &worldPos, const voxel::RawVolume *volume) const {
		const voxel::Region &region = volume->region();
		const glm::ivec3 voxelPos = glm::floor(worldPos);
		if (!region.containsPoint(voxelPos)) {
			return false;
		}
		const voxel::Voxel &vox = volume->voxel(voxelPos);
		return !voxel::isAir(vox.getMaterial());
	}

	glm::vec3 attemptMovement(const char *command, const voxel::RawVolume *volume, CameraMovementExt &m,
							  video::Camera &camera, scenegraph::SceneGraph &sceneGraph, const glm::vec3 &startPos) {
		camera.setWorldPosition(startPos);
		camera.update(0.0);
		m.updateBodyPosition(camera);
		EXPECT_FALSE(isInsideSolid(startPos, volume)) << "Start position should be outside solids";
		double nowSeconds = 0.0;
		command::ScopedButtonCommand pressed(command, 10, 0.0);
		const scenegraph::FrameIndex frameIdx = 0;
		for (int step = 0; step < 20; ++step) {
			nowSeconds += 0.016;
			m.update(nowSeconds, &camera, sceneGraph, frameIdx);
			camera.update(0.0);
		}
		const glm::vec3 pos = camera.worldPosition();
		EXPECT_FALSE(isInsideSolid(pos, volume)) << "Camera ended inside solid voxels for command '" << command << "' "
												 << pos.x << "," << pos.y << "," << pos.z;
		return pos;
	}

	void prepareSolidSceneGraph(scenegraph::SceneGraph &sceneGraph) {
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("solidModel");
		voxel::Region region(-8, 8);
		core_assert(region.isValid());
		voxel::RawVolume *v = new voxel::RawVolume(region);
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
					v->setVoxel(x, y, z, voxel);
				}
			}
		}
		node.setVolume(v, true);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node)));
	}

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

TEST_F(CameraMovementTest, testClippingPreventsEnteringSolidVolume) {
	CameraMovementExt m;
	m.construct();
	util::ScopedVarChange scoped(cfg::GameModeClipping, "true");
	ASSERT_TRUE(m.init());

	scenegraph::SceneGraph sceneGraph;
	prepareSolidSceneGraph(sceneGraph);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const voxel::RawVolume *volume = node->volume();
	ASSERT_NE(nullptr, volume);
	const voxel::Region &region = volume->region();

	video::Camera camera;
	camera.setRotationType(video::CameraRotationType::Eye);
	camera.setSize({800, 600});

	const glm::vec3 volumeCenter = region.calcCenterf();
	const float planeY = volumeCenter.y;

	const float positiveFaceX = region.getUpperX() + 1.0f;
	const float negativeFaceX = region.getLowerX();
	const float positiveFaceZ = region.getUpperZ() + 1.0f;
	const float negativeFaceZ = region.getLowerZ();
	const float clearance = 0.499999999f;
	const float tolerance = 0.002f;

	const glm::vec3 left = attemptMovement("move_left", volume, m, camera, sceneGraph,
										   glm::vec3(positiveFaceX + 2.0f, planeY, volumeCenter.z));
	EXPECT_GE(left.x, positiveFaceX + tolerance);
	EXPECT_GE(glm::abs(left.x - positiveFaceX), clearance - tolerance);
	EXPECT_LE(left.x, positiveFaceX + 2.0f);

	const glm::vec3 right = attemptMovement("move_right", volume, m, camera, sceneGraph,
											glm::vec3(negativeFaceX - 2.0f, planeY, volumeCenter.z));
	// EXPECT_LE(right.x, negativeFaceX - tolerance);
	EXPECT_GE(glm::abs(right.x - negativeFaceX), clearance - tolerance);
	EXPECT_GE(right.x, negativeFaceX - 2.0f);

	const glm::vec3 forward = attemptMovement("move_forward", volume, m, camera, sceneGraph,
											  glm::vec3(volumeCenter.x, planeY, positiveFaceZ + 2.0f));
	// EXPECT_GE(forward.z, positiveFaceZ + tolerance);
	EXPECT_GE(glm::abs(forward.z - positiveFaceZ), clearance - tolerance);
	EXPECT_LE(forward.z, positiveFaceZ + 2.0f);

	const glm::vec3 backward = attemptMovement("move_backward", volume, m, camera, sceneGraph,
											   glm::vec3(volumeCenter.x, planeY, negativeFaceZ - 2.0f));
	// EXPECT_LE(backward.z, negativeFaceZ - tolerance);
	EXPECT_GE(glm::abs(backward.z - negativeFaceZ), clearance - tolerance);
	EXPECT_GE(backward.z, negativeFaceZ - 2.0f);

	m.shutdown();
}

} // namespace voxedit
