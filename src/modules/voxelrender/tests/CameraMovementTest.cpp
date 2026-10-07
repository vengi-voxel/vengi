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
#include <gtest/gtest.h>

namespace voxelrender {

class CameraMovementTest : public app::AbstractTest {
protected:
	class CameraMovementExt : public CameraMovement {
	public:
		util::Movement &movement() {
			return _movement;
		}
	};

	void SetUp() override {
		app::AbstractTest::SetUp();
		const core::VarDef clientMouseRotationSpeed(cfg::ClientMouseRotationSpeed, 0.01f, "", "");
		core::Var::registerVar(clientMouseRotationSpeed);
		const core::VarDef clientCameraZoomSpeed(cfg::ClientCameraZoomSpeed, 0.1f, "", "");
		core::Var::registerVar(clientCameraZoomSpeed);
		core::Var::registerVar(core::VarDef(cfg::ClientCameraMinZoom, 0.001f, "", ""));
		core::Var::registerVar(core::VarDef(cfg::ClientCameraMaxZoom, 1000.0f, "", ""));
	}

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
		node.setVolume(v);
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
		node.setVolume(v);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node)));
	}
};

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

TEST_F(CameraMovementTest, testOrthogonalRotationAllowsPitch) {
	CameraMovementExt m;
	m.construct();

	video::Camera camera;
	camera.setMode(video::CameraMode::Orthogonal);
	camera.setSize({800, 600});
	camera.setAngles(0.0f, 0.0f, 0.0f);
	camera.update(0.0);

	m.rotate(camera, 20.0f, 20.0f);
	camera.update(0.0);

	EXPECT_GT(glm::length(camera.forward() - glm::forward()), 0.01f);
	EXPECT_GT(glm::abs(camera.horizontalYaw()), 0.1f);
}

TEST_F(CameraMovementTest, testIsometricRotationKeepsPitch) {
	CameraMovementExt m;
	m.construct();

	video::Camera camera;
	camera.setMode(video::CameraMode::Isometric);
	camera.setSize({800, 600});
	camera.setAngles(video::Camera::IsometricPitch, 0.0f, 0.0f);
	camera.update(0.0);
	const glm::vec3 initialForward = camera.forward();

	m.rotate(camera, 20.0f, 20.0f);
	camera.update(0.0);

	EXPECT_NEAR(initialForward.y, camera.forward().y, 0.02f);
	EXPECT_GT(glm::abs(camera.horizontalYaw()), 0.1f);
	EXPECT_GT(glm::length(camera.forward() - initialForward), 0.01f);
}

TEST_F(CameraMovementTest, navigationMovementAndDollyMatchAtEqualFraming) {
	const video::CameraMode modes[] = {video::CameraMode::Perspective, video::CameraMode::Orthogonal, video::CameraMode::Isometric};
	const video::CameraRotationType rotations[] = {video::CameraRotationType::Target, video::CameraRotationType::Eye};
	for (video::CameraMode mode : modes) {
		for (video::CameraRotationType rotation : rotations) {
			for (float distance : {50.0f, 500.0f}) {
				for (const char *button : {"move_left", "move_forward", "move_backward"}) {
					CameraMovementExt m;
					m.construct();
					ASSERT_TRUE(m.init());
					video::Camera camera;
					camera.setSize({1000, 800});
					camera.setTarget(glm::vec3(0));
					camera.setWorldPosition({0, 0, distance});
					camera.setTargetDistance(distance);
					camera.setMode(mode);
					if (camera.isOrthographic()) {
						const float zoom = 2 * distance * glm::tan(glm::radians(45.0f) / 2) / (800 * camera.worldUnitsPerPixel());
						camera.zoom(glm::log(zoom) / 0.1f);
					}
					camera.setRotationType(rotation);
					camera.update(0);
					const glm::vec3 landmark = camera.target() + camera.right() * 10.0f;
					const glm::ivec2 before = camera.worldToScreen(landmark);
					scenegraph::SceneGraph graph;
					m.update(0, &camera, graph, 0);
					{
						command::ScopedButtonCommand pressed(button, 10, 0);
						for (int frame = 1; frame <= 60; ++frame) {
							m.update(frame / 60.0, &camera, graph, 0);
							camera.update(0);
						}
					}
					const glm::ivec2 after = camera.worldToScreen(landmark);
					if (core::String(button) == "move_left") {
						EXPECT_NEAR(after.x - before.x, 60, 1);
					} else {
						const float rate = 2 * glm::tan(glm::radians(45.0f) / 2) * 60 / 800;
						const float sign = core::String(button) == "move_forward" ? 1 : -1;
						const float initialPixels = 10 / (2 * distance * glm::tan(glm::radians(45.0f) / 2) / 800);
						EXPECT_NEAR(glm::abs(after.x - 500), initialPixels * glm::exp(sign * rate), 1.1f);
					}
					m.shutdown();
				}
			}
		}
	}
}

TEST_F(CameraMovementTest, wheelZoomHasEqualRelativeResponse) {
	for (video::CameraMode mode : {video::CameraMode::Perspective, video::CameraMode::Orthogonal}) {
		for (video::CameraRotationType rotation : {video::CameraRotationType::Target, video::CameraRotationType::Eye}) {
			CameraMovementExt m;
			m.construct();
			video::Camera camera;
			camera.setSize({1000, 800});
			camera.setWorldPosition({0, 0, 100});
			camera.setTargetDistance(100);
			camera.setMode(mode);
			camera.setRotationType(rotation);
			camera.update(0);
			const glm::ivec2 before = camera.worldToScreen({10, 0, 0});
			m.zoom(camera, -1);
			camera.update(0);
			const glm::ivec2 after = camera.worldToScreen({10, 0, 0});
			EXPECT_NEAR(glm::abs(after.x - 500), glm::abs(before.x - 500) * glm::exp(0.1f), 1.1f);
		}
	}
}

TEST_F(CameraMovementTest, navigationSpeedAndSprintSettingsScaleResponse) {
	for (const char *button : {"move_left", "move_forward"}) {
		float responses[3];
		for (int run = 0; run < 3; ++run) {
			CameraMovementExt m;
			m.construct();
			ASSERT_TRUE(m.init());
			util::ScopedVarChange speed(cfg::GameModeMovementSpeed, run == 1 ? "120" : "60");
			util::ScopedVarChange clipping(cfg::GameModeClipping, "false");
			util::ScopedVarChange multiplier(cfg::GameModeSprintMultiplier, "3");
			video::Camera camera;
			camera.setSize({1000, 800});
			camera.setTarget(glm::vec3(0));
			camera.setTargetDistance(100);
			camera.update(0);
			scenegraph::SceneGraph graph;
			m.update(0, &camera, graph, 0);
			const glm::vec3 before = camera.worldPosition();
			{
				command::ScopedButtonCommand pressed(button, 10, 0);
				if (run == 2) {
					command::ScopedButtonCommand sprint("sprint", 11, 0);
					m.update(0.1, &camera, graph, 0);
				} else {
					m.update(0.1, &camera, graph, 0);
				}
			}
			camera.update(0);
			responses[run] = core::String(button) == "move_left"
				? glm::distance(before, camera.worldPosition())
				: glm::log(100.0f / camera.targetDistance());
			m.shutdown();
		}
		ASSERT_GT(responses[0], 0.0f);
		EXPECT_NEAR(responses[1] / responses[0], 2.0f, 0.001f);
		EXPECT_NEAR(responses[2] / responses[0], 3.0f, 0.001f);
	}
}

} // namespace voxelrender
