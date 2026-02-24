/**
 * @file
 */

#include "video/Camera.h"
#include "app/tests/AbstractTest.h"
#include "core/ConfigVar.h"
#include "core/GLM.h"
#include "core/Var.h"
#include "math/Frustum.h"
#include "math/Ray.h"
#include "util/VarUtil.h"

namespace video {

class CameraTest : public app::AbstractTest {
public:
	virtual ~CameraTest() {
	}

protected:
	core::VarPtr _zoomSpeed;
	core::VarPtr _maxZoom;
	core::VarPtr _minZoom;

	// looking straight down is the default
	Camera setup(const glm::vec2 &dimension = glm::vec2(1024, 768),
				 const glm::vec3 &position = glm::vec3(0.0, 1.0, 0.0), const glm::vec3 &lookAt = glm::vec3(0.0),
				 const glm::vec3 &lookAlong = glm::forward()) {
		Camera camera;
		camera.setNearPlane(0.1f);
		camera.setFarPlane(100.0f);
		camera.setSize(dimension);
		camera.setWorldPosition(position);
		camera.lookAt(lookAt, lookAlong);
		camera.update(0.0);
		return camera;
	}

	void SetUp() override {
		const core::VarDef clientCameraZoomSpeed(cfg::ClientCameraZoomSpeed, 0.1f);
		_zoomSpeed = core::Var::registerVar(clientCameraZoomSpeed);
		const core::VarDef clientCameraMaxZoom(cfg::ClientCameraMaxZoom, 1000.0f);
		_maxZoom = core::Var::registerVar(clientCameraMaxZoom);
		const core::VarDef clientCameraMinZoom(cfg::ClientCameraMinZoom, 0.001f);
		_minZoom = core::Var::registerVar(clientCameraMinZoom);
	}
};

TEST_F(CameraTest, testLookAt) {
	Camera camera = setup();
	EXPECT_FLOAT_EQ(glm::half_pi<float>(), camera.pitch());
	EXPECT_FLOAT_EQ(0.0f, camera.yaw());
	EXPECT_FLOAT_EQ(0.0f, camera.roll());
	const glm::vec4 invecs[6] = {
		// left bottom, right bottom, right top
		glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f), glm::vec4( 1.0f, -1.0f, 0.0f, 1.0f), glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f),
		// left bottom, right top, left top
		glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f), glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f), glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f)
	};

	glm::vec4 outvecs[6];
	for (int i = 0; i < 6; ++i) {
		outvecs[i] = camera.viewProjectionMatrix() * invecs[i];
	}

	EXPECT_FLOAT_EQ(outvecs[0].y, outvecs[1].y) << "left bottom - right bottom y is invalid";
	EXPECT_FLOAT_EQ(outvecs[2].y, outvecs[5].y) << "right top - left top y is invalid";

	EXPECT_FLOAT_EQ(outvecs[0].x, outvecs[5].x) << "left bottom - left top x is invalid";
	EXPECT_FLOAT_EQ(outvecs[1].x, outvecs[4].x) << "right bottom - right top x is invalid";

	EXPECT_LT(outvecs[2].y, outvecs[1].y) << "right top - right bottom y is invalid - maybe a sign error";
	EXPECT_LT(outvecs[5].y, outvecs[0].y) << "left top - left bottom y is invalid - maybe a sign error";
}

TEST_F(CameraTest, testMotion) {
	// TODO: finish this
	Camera camera = setup(glm::vec2(1024, 768), glm::vec3(0.0, 1.0, 0.0));
	camera.rotate(glm::vec3(0.0f, 10.0f, 0.0f));
	camera.update(0.0);
}

TEST_F(CameraTest, testParallelLookAt) {
	Camera camera = setup(glm::vec2(1024, 768), glm::vec3(0.0, 10.0, 0.0), glm::vec3(0.0), glm::up());
	camera.update(0.0);
	const glm::vec4 invecs[6] = {
		// left bottom, right bottom, right top
		glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f), glm::vec4( 1.0f, -1.0f, 0.0f, 1.0f), glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f),
		// left bottom, right top, left top
		glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f), glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f), glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f)
	};

	glm::vec4 outvecs[6];
	for (int i = 0; i < 6; ++i) {
		outvecs[i] = camera.viewProjectionMatrix() * invecs[i];
	}

	EXPECT_FLOAT_EQ(outvecs[0].y, outvecs[1].y) << "left bottom - right bottom y is invalid";
	EXPECT_FLOAT_EQ(outvecs[2].y, outvecs[5].y) << "right top - left top y is invalid";

	EXPECT_FLOAT_EQ(outvecs[0].x, outvecs[5].x) << "left bottom - left top x is invalid";
	EXPECT_FLOAT_EQ(outvecs[1].x, outvecs[4].x) << "right bottom - right top x is invalid";

	EXPECT_LT(outvecs[2].y, outvecs[1].y) << "right top - right bottom y is invalid - maybe a sign error";
	EXPECT_LT(outvecs[5].y, outvecs[0].y) << "left top - left bottom y is invalid - maybe a sign error";
}

TEST_F(CameraTest, testCameraFrustumCullingPerspective) {
	Camera camera = setup(glm::vec2(1024, 768), glm::vec3(0.1, 1.0, 0.1));
	camera.setMode(CameraMode::Perspective);
	camera.update(0.0);
	const math::Frustum &frustum = camera.frustum();
	EXPECT_EQ(math::FrustumResult::Inside, frustum.test(glm::vec3(0.0, 0.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Outside, frustum.test(glm::vec3(0.0, 1.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Intersect, frustum.test(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(0.5, 0.5, 0.5)));
}

TEST_F(CameraTest, testMoveAndPan) {
	Camera camera = setup(glm::vec2(1024, 768), glm::vec3(0.0, 1.0, 0.0));
	const glm::vec3 before = camera.worldPosition();

	// moving along +z in camera-space should change world position
	const bool moved = camera.move(glm::vec3(0.0f, 0.0f, 1.0f));
	EXPECT_TRUE(moved);
	const glm::vec3 after = camera.worldPosition();
	// For the default look-at (looking down) moving +z moves the camera up in world Y
	EXPECT_NE(before, after);

	// panning should change panOffset
	const glm::vec3 panBefore = camera.worldPosition();
	camera.pan(10, 20);
	const glm::vec3 panAfter = camera.worldPosition();
	EXPECT_NE(panBefore, panAfter);
}

TEST_F(CameraTest, testSlerp) {
	Camera camera = setup();
	const glm::quat startQuat = camera.quaternion();
	// rotate target by 90 degrees around Y
	const glm::quat targetQuat = glm::angleAxis(glm::half_pi<float>(), glm::up());
	camera.slerp(targetQuat, 0.5f);
	const glm::quat midQuat = camera.quaternion();
	// Expect quaternion to have changed but not equal to target yet
	EXPECT_FALSE(glm::all(glm::epsilonEqual(startQuat, midQuat, 0.0001f)));
	EXPECT_FALSE(glm::all(glm::epsilonEqual(targetQuat, midQuat, 0.0001f)));
}

TEST_F(CameraTest, testLerpBetweenCameras) {
	Camera a = setup(glm::vec2(200, 200), glm::vec3(0.0f, 10.0f, 0.0));
	Camera b = setup(glm::vec2(200, 200), glm::vec3(10.0f, 10.0f, 0.0));
	// ensure target is clean before lerp
	b.update(0.0);
	const glm::vec3 aBefore = a.worldPosition();
	const glm::vec3 bPos = b.worldPosition();
	a.lerp(b);
	// progress half-way
	a.update(0.5);
	const glm::vec3 aMid = a.worldPosition();
	EXPECT_FALSE(glm::all(glm::epsilonEqual(aBefore, aMid, 0.0001f)));
	// finish lerp
	a.update(1.0);
	const glm::vec3 aAfter = a.worldPosition();
	// after finishing lerp the world position should be (approximately) equal to target
	EXPECT_NEAR(bPos.x, aAfter.x, 0.001f);
	EXPECT_NEAR(bPos.y, aAfter.y, 0.001f);
	EXPECT_NEAR(bPos.z, aAfter.z, 0.001f);
}

TEST_F(CameraTest, testCameraFrustumCullingOrthogonal) {
	Camera camera;
	camera.setSize(glm::vec2(100.0f, 100.0f));
	camera.setMode(CameraMode::Orthogonal);
	camera.setOrthoDepth(10.0f);
	camera.setWorldPosition(glm::vec3(0.1, 1.0, 0.1));
	camera.lookAt(glm::vec3(0.0), glm::forward());
	camera.update(0.0);
	const math::Frustum &frustum = camera.frustum();
	EXPECT_EQ(math::FrustumResult::Inside, frustum.test(glm::vec3(0.0, 0.0, 0.0)));
	// A point far above the camera should be outside the frustum when looking down
	EXPECT_EQ(math::FrustumResult::Outside, frustum.test(glm::vec3(0.0, 10.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Inside, frustum.test(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(0.5, 0.5, 0.5)));
	// TODO: add math::FrustumResult::Intersect test
}

TEST_F(CameraTest, testOrthoZoom) {
	util::ScopedVarChange zoomSpeedChange(_zoomSpeed->name(), 0.9f);
	Camera camera;
	camera.setMode(CameraMode::Perspective);
	camera.setSize(glm::ivec2(1024, 768));
	camera.setMode(CameraMode::Orthogonal);
	camera.update(0.0);

	const float initialZoom = camera.orthoZoom();

	camera.zoom(1.0f);
	const float zoomedOut = camera.orthoZoom();
	EXPECT_GT(zoomedOut, initialZoom);
	EXPECT_LE(zoomedOut, _maxZoom->floatVal());

	const float expectedFactor = glm::exp(_zoomSpeed->floatVal());
	EXPECT_NEAR(zoomedOut, initialZoom * expectedFactor, 0.001f) << "Zoom out should match configured speed";

	camera.zoom(-1.0f);
	const float zoomedIn = camera.orthoZoom();
	EXPECT_FLOAT_EQ(zoomedIn, initialZoom);
	EXPECT_GE(zoomedIn, _minZoom->floatVal());
}

TEST_F(CameraTest, testWorldToScreen) {
	Camera camera = setup(glm::vec2(100, 100), glm::vec3(0.0, 0.0, 10.0), glm::vec3(0.0, 0.0, 0.0), glm::up());

	const glm::ivec2 center = camera.worldToScreen(glm::vec3(0.0, 0.0, 0.0));
	EXPECT_EQ(50, center.x);
	EXPECT_EQ(50, center.y);

	const glm::ivec2 up = camera.worldToScreen(glm::up());
	EXPECT_LT(up.y, center.y);
	EXPECT_EQ(up.x, center.x);

	const glm::ivec2 right = camera.worldToScreen(glm::right());
	EXPECT_GT(right.x, center.x);
	EXPECT_EQ(right.y, center.y);
}

TEST_F(CameraTest, testMouseRay) {
	Camera camera = setup(glm::vec2(100, 100), glm::vec3(0.0, 0.0, 10.0), glm::vec3(0.0, 0.0, 0.0), glm::up());

	const math::Ray& ray = camera.mouseRay(glm::ivec2(50, 50));
	const glm::vec3& dir = ray.direction;
	EXPECT_NEAR(0.0f, dir.x, 0.001f);
	EXPECT_NEAR(0.0f, dir.y, 0.001f);
	EXPECT_NEAR(-1.0f, dir.z, 0.001f);

	const math::Ray& ray2 = camera.mouseRay(glm::ivec2(50, 0));
	EXPECT_GT(ray2.direction.y, 0.0f);
	EXPECT_NEAR(0.0f, ray2.direction.x, 0.001f);

	const math::Ray& ray3 = camera.mouseRay(glm::ivec2(100, 50));
	EXPECT_GT(ray3.direction.x, 0.0f);
	EXPECT_NEAR(0.0f, ray3.direction.y, 0.001f);
}

TEST_F(CameraTest, testBillboard) {
	Camera camera = setup(glm::vec2(100, 100), glm::vec3(0.0, 0.0, 10.0), glm::vec3(0.0, 0.0, 0.0), glm::up());
	glm::vec3 right, up;
	camera.billboard(&right, &up);

	EXPECT_NEAR(1.0f, right.x, 0.001f);
	EXPECT_NEAR(0.0f, right.y, 0.001f);
	EXPECT_NEAR(0.0f, right.z, 0.001f);

	EXPECT_NEAR(0.0f, up.x, 0.001f);
	EXPECT_NEAR(1.0f, up.y, 0.001f);
	EXPECT_NEAR(0.0f, up.z, 0.001f);
}

} // namespace video
