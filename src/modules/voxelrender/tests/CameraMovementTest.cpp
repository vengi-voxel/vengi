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
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec2i.h"
#include "math/Axes.h"
#include "math/Scalar.h"

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

	bool isInsideSolid(const math::Vec3f &worldPos, const voxel::RawVolume *volume) const {
		const voxel::Region &region = volume->region();
		const math::Vec3i voxelPos = math::floor(worldPos);
		if (!region.containsPoint(voxelPos)) {
			return false;
		}
		const voxel::Voxel &vox = volume->voxel(voxelPos);
		return !voxel::isAir(vox.getMaterial());
	}

	math::Vec3f attemptMovement(const char *command, const voxel::RawVolume *volume, CameraMovementExt &m,
							  video::Camera &camera, scenegraph::SceneGraph &sceneGraph, const math::Vec3f &startPos) {
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
		const math::Vec3f pos = camera.worldPosition();
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

	const math::Vec3f volumeCenter = region.calcCenterf();
	const float planeY = volumeCenter.y;

	const float positiveFaceX = region.getUpperX() + 1.0f;
	const float negativeFaceX = region.getLowerX();
	const float positiveFaceZ = region.getUpperZ() + 1.0f;
	const float negativeFaceZ = region.getLowerZ();
	const float clearance = 0.499999999f;
	const float tolerance = 0.002f;

	const math::Vec3f left = attemptMovement("move_left", volume, m, camera, sceneGraph,
										   math::Vec3f(positiveFaceX + 2.0f, planeY, volumeCenter.z));
	EXPECT_GE(left.x, positiveFaceX + tolerance);
	EXPECT_GE(math::abs(left.x - positiveFaceX), clearance - tolerance);
	EXPECT_LE(left.x, positiveFaceX + 2.0f);

	const math::Vec3f right = attemptMovement("move_right", volume, m, camera, sceneGraph,
											math::Vec3f(negativeFaceX - 2.0f, planeY, volumeCenter.z));
	// EXPECT_LE(right.x, negativeFaceX - tolerance);
	EXPECT_GE(math::abs(right.x - negativeFaceX), clearance - tolerance);
	EXPECT_GE(right.x, negativeFaceX - 2.0f);

	const math::Vec3f forward = attemptMovement("move_forward", volume, m, camera, sceneGraph,
											  math::Vec3f(volumeCenter.x, planeY, positiveFaceZ + 2.0f));
	// EXPECT_GE(forward.z, positiveFaceZ + tolerance);
	EXPECT_GE(math::abs(forward.z - positiveFaceZ), clearance - tolerance);
	EXPECT_LE(forward.z, positiveFaceZ + 2.0f);

	const math::Vec3f backward = attemptMovement("move_backward", volume, m, camera, sceneGraph,
											   math::Vec3f(volumeCenter.x, planeY, negativeFaceZ - 2.0f));
	// EXPECT_LE(backward.z, negativeFaceZ - tolerance);
	EXPECT_GE(math::abs(backward.z - negativeFaceZ), clearance - tolerance);
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

	EXPECT_GT(math::length(camera.forward() - math::forward()), 0.01f);
	EXPECT_GT(math::abs(camera.horizontalYaw()), 0.1f);
}

TEST_F(CameraMovementTest, testIsometricRotationKeepsPitch) {
	CameraMovementExt m;
	m.construct();

	video::Camera camera;
	camera.setMode(video::CameraMode::Isometric);
	camera.setSize({800, 600});
	camera.setAngles(video::Camera::IsometricPitch, 0.0f, 0.0f);
	camera.update(0.0);
	const math::Vec3f initialForward = camera.forward();

	m.rotate(camera, 20.0f, 20.0f);
	camera.update(0.0);

	EXPECT_NEAR(initialForward.y, camera.forward().y, 0.02f);
	EXPECT_GT(math::abs(camera.horizontalYaw()), 0.1f);
	EXPECT_GT(math::length(camera.forward() - initialForward), 0.01f);
}

TEST_F(CameraMovementTest, navigationMovementMatchesScreenPixelsAtEqualFraming) {
	CameraMovementExt setup;
	setup.construct();
	util::ScopedVarChange speed(cfg::ClientCameraMovementSpeed, "60");
	const video::CameraMode modes[] = {video::CameraMode::Perspective, video::CameraMode::Orthogonal, video::CameraMode::Isometric};
	const video::CameraRotationType rotations[] = {video::CameraRotationType::Target, video::CameraRotationType::Eye};
	for (video::CameraMode mode : modes) {
		for (video::CameraRotationType rotation : rotations) {
			for (float distance : {50.0f, 500.0f}) {
				for (const char *button : {"move_left", "move_right", "move_forward", "move_backward"}) {
					CameraMovementExt m;
					m.construct();
					ASSERT_TRUE(m.init());
					video::Camera camera;
					camera.setSize({1000, 800});
					camera.setTarget(math::Vec3f(0));
					camera.setWorldPosition({0, 0, distance});
					camera.setTargetDistance(distance);
					camera.setMode(mode);
					if (camera.isOrthographic()) {
						const float zoom = 2 * distance * math::tan(math::radians(45.0f) / 2) / (800 * camera.worldUnitsPerPixel());
						camera.zoom(math::log(zoom) / 0.1f);
					}
					camera.setRotationType(rotation);
					camera.update(0);
					const float wupp = camera.worldUnitsPerPixel();
					const math::Vec3f beforePos = camera.worldPosition();
					const math::Vec3f landmark = camera.target() + camera.right() * 10.0f;
					const math::Vec2i before = camera.worldToScreen(landmark);
					scenegraph::SceneGraph graph;
					m.update(0, &camera, graph, 0);
					{
						command::ScopedButtonCommand pressed(button, 10, 0);
						for (int frame = 1; frame <= 60; ++frame) {
							m.update(frame / 60.0, &camera, graph, 0);
							camera.update(0);
						}
					}
					const core::String btn(button);
					if (btn == "move_left" || btn == "move_right") {
						const math::Vec2i after = camera.worldToScreen(landmark);
						const float expected = btn == "move_left" ? 60.0f : -60.0f;
						EXPECT_NEAR(after.x - before.x, expected, 1);
					} else {
						EXPECT_NEAR(math::distance(beforePos, camera.worldPosition()), 60.0f * wupp, 0.05f);
					}
					EXPECT_NEAR(camera.targetDistance(), distance, 0.01f);
					m.shutdown();
				}
			}
		}
	}
}

TEST_F(CameraMovementTest, editorForwardKeepsConstantWorldSpeedAndDistance) {
	// Regression for #904: WASD must not slow down as you fly farther into a scene.
	CameraMovementExt setup;
	setup.construct();
	util::ScopedVarChange speed(cfg::ClientCameraMovementSpeed, "60");
	util::ScopedVarChange clipping(cfg::GameModeClipping, "false");
	for (video::CameraRotationType rotation : {video::CameraRotationType::Target, video::CameraRotationType::Eye}) {
		CameraMovementExt m;
		m.construct();
		ASSERT_TRUE(m.init());
		video::Camera camera;
		camera.setSize({1000, 800});
		camera.setTarget(math::Vec3f(0));
		camera.setWorldPosition({0, 0, 200});
		camera.setTargetDistance(200);
		camera.setRotationType(rotation);
		camera.update(0);
		scenegraph::SceneGraph graph;
		m.update(0, &camera, graph, 0);
		const float startDistance = camera.targetDistance();
		const float wupp = camera.worldUnitsPerPixel();
		float firstHalf = 0.0f;
		float secondHalf = 0.0f;
		{
			command::ScopedButtonCommand pressed("move_forward", 10, 0);
			math::Vec3f prev = camera.worldPosition();
			for (int frame = 1; frame <= 120; ++frame) {
				m.update(frame / 60.0, &camera, graph, 0);
				camera.update(0);
				const float step = math::distance(prev, camera.worldPosition());
				if (frame <= 60) {
					firstHalf += step;
				} else {
					secondHalf += step;
				}
				prev = camera.worldPosition();
			}
		}
		EXPECT_NEAR(camera.targetDistance(), startDistance, 0.01f) << "rotation " << (int)rotation;
		EXPECT_NEAR(camera.worldUnitsPerPixel(), wupp, 0.0001f) << "rotation " << (int)rotation;
		EXPECT_NEAR(firstHalf, 60.0f * wupp, 0.05f) << "rotation " << (int)rotation;
		EXPECT_NEAR(secondHalf, firstHalf, 0.05f) << "rotation " << (int)rotation;
		m.shutdown();
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
			const math::Vec2i before = camera.worldToScreen({10, 0, 0});
			m.zoom(camera, -1);
			camera.update(0);
			const math::Vec2i after = camera.worldToScreen({10, 0, 0});
			EXPECT_NEAR(math::abs(after.x - 500), math::abs(before.x - 500) * math::exp(0.1f), 1.1f);
		}
	}
}

TEST_F(CameraMovementTest, perspectiveWheelKeepsMovingPastTarget) {
	for (video::CameraRotationType rotation : {video::CameraRotationType::Target, video::CameraRotationType::Eye}) {
		CameraMovementExt m;
		m.construct();
		util::ScopedVarChange clipping(cfg::GameModeClipping, "false");
		video::Camera camera;
		camera.setSize({1000, 800});
		camera.setTarget(math::Vec3f(0));
		camera.setWorldPosition({0, 0, 100});
		camera.setTargetDistance(100);
		camera.setRotationType(rotation);
		camera.update(0);
		const math::Vec3f forward = camera.forward();
		const float wupp = camera.worldUnitsPerPixel();
		float firstStep = 0.0f;
		for (int step = 0; step < 200; ++step) {
			const math::Vec3f before = camera.worldPosition();
			m.zoom(camera, -1);
			camera.update(0);
			const float distance = math::dot(camera.worldPosition() - before, forward);
			if (step == 0) {
				firstStep = distance;
				ASSERT_GT(firstStep, 0.0f);
			}
			EXPECT_NEAR(distance, firstStep, 0.001f) << "step " << step << " rotation " << (int)rotation;
		}
		EXPECT_LT(camera.worldPosition().z, 0.0f);
		EXPECT_NEAR(camera.targetDistance(), 100.0f, 0.001f);
		EXPECT_NEAR(camera.worldUnitsPerPixel(), wupp, 0.0001f);
		EXPECT_LT(math::distance(camera.forward(), forward), 0.001f);
		const math::Vec3f before = camera.worldPosition();
		m.zoom(camera, 1);
		camera.update(0);
		EXPECT_NEAR(math::dot(before - camera.worldPosition(), forward), 100.0f * (math::exp(0.1f) - 1.0f), 0.001f);
		EXPECT_NEAR(camera.targetDistance(), 100.0f, 0.001f);
		const math::Vec3f after = camera.worldPosition();
		util::ScopedVarChange enableClipping(cfg::GameModeClipping, "true");
		m.zoom(camera, -1);
		camera.update(0);
		EXPECT_FLOAT_EQ(math::distance(camera.worldPosition(), after), 0.0f);
	}
}

TEST_F(CameraMovementTest, navigationSpeedScalesResponseAndIgnoresSprint) {
	for (const char *button : {"move_left", "move_forward"}) {
		float responses[3];
		for (int run = 0; run < 3; ++run) {
			CameraMovementExt m;
			m.construct();
			ASSERT_TRUE(m.init());
			util::ScopedVarChange speed(cfg::ClientCameraMovementSpeed, run == 1 ? "120" : "60");
			util::ScopedVarChange clipping(cfg::GameModeClipping, "false");
			util::ScopedVarChange multiplier(cfg::GameModeSprintMultiplier, "3");
			video::Camera camera;
			camera.setSize({1000, 800});
			camera.setTarget(math::Vec3f(0));
			camera.setWorldPosition({0, 0, 100});
			camera.setTargetDistance(100);
			camera.update(0);
			scenegraph::SceneGraph graph;
			m.update(0, &camera, graph, 0);
			const math::Vec3f before = camera.worldPosition();
			const float beforeDistance = camera.targetDistance();
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
			responses[run] = math::distance(before, camera.worldPosition());
			EXPECT_NEAR(camera.targetDistance(), beforeDistance, 0.01f);
			m.shutdown();
		}
		ASSERT_GT(responses[0], 0.0f);
		EXPECT_NEAR(responses[1] / responses[0], 2.0f, 0.001f);
		EXPECT_NEAR(responses[2] / responses[0], 1.0f, 0.001f);
	}
}

TEST_F(CameraMovementTest, editorAndGameMovementSpeedsAreIndependent) {
	CameraMovementExt setup;
	setup.construct();
	EXPECT_FLOAT_EQ(core::getVar(cfg::ClientCameraMovementSpeed)->floatVal(), 200.0f);
	EXPECT_FLOAT_EQ(core::getVar(cfg::GameModeMovementSpeed)->floatVal(), 60.0f);
	for (bool clipping : {false, true}) {
		float response[4];
		for (int run = 0; run < 4; ++run) {
			CameraMovementExt m;
			m.construct();
			ASSERT_TRUE(m.init());
			util::ScopedVarChange editorSpeed(cfg::ClientCameraMovementSpeed, run == 1 ? "400" : "200");
			util::ScopedVarChange gameSpeed(cfg::GameModeMovementSpeed, run == 2 ? "120" : "60");
			util::ScopedVarChange clippingSetting(cfg::GameModeClipping, clipping ? "true" : "false");
			util::ScopedVarChange gravity(cfg::GameModeApplyGravity, "false");
			video::Camera camera;
			camera.setSize({1000, 800});
			camera.setTargetDistance(100);
			camera.update(0);
			camera.setRotationType(video::CameraRotationType::Eye);
			camera.setWorldPosition({8, 5, 8});
			camera.update(0);
			m.updateBodyPosition(camera);
			scenegraph::SceneGraph graph;
			prepareSceneGraph(graph);
			m.update(0, &camera, graph, 0);
			const float before = camera.worldPosition().x;
			{
				command::ScopedButtonCommand pressed("move_left", 10, 0);
				auto advance = [&]() {
					for (int frame = 1; frame <= 30; ++frame) {
						m.update(frame / 60.0, &camera, graph, 0);
						camera.update(0);
					}
				};
				if (run == 3) {
					command::ScopedButtonCommand sprint("sprint", 11, 0);
					advance();
				} else {
					advance();
				}
			}
			response[run] = math::abs(camera.worldPosition().x - before);
			m.shutdown();
		}
		ASSERT_GT(response[0], 0.0f);
		EXPECT_NEAR(response[1] / response[0], clipping ? 1.0f : 2.0f, 0.001f);
		EXPECT_NEAR(response[2] / response[0], clipping ? 2.0f : 1.0f, 0.001f);
		EXPECT_NEAR(response[3] / response[0], clipping ? 3.0f : 1.0f, 0.001f);
	}
}

} // namespace voxelrender
