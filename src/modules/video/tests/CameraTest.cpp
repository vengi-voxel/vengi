/**
 * @file
 */

#include "video/Camera.h"
#include "app/tests/AbstractTest.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "math/Frustum.h"
#include "math/Ray.h"
#include "util/VarUtil.h"
#include "video/Renderer.h"
#include "math/Quatf.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"
#include "math/Vec2i.h"
#include "math/Vec2f.h"
#include "math/Axes.h"
#include "math/Scalar.h"

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
	Camera setup(const math::Vec2f &dimension = math::Vec2f(1024, 768),
				 const math::Vec3f &position = math::Vec3f(0.0, 1.0, 0.0), const math::Vec3f &lookAt = math::Vec3f(0.0),
				 const math::Vec3f &lookAlong = math::forward()) {
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
		const core::VarDef clientCameraZoomSpeed(cfg::ClientCameraZoomSpeed, 0.1f, "", "");
		_zoomSpeed = core::Var::registerVar(clientCameraZoomSpeed);
		const core::VarDef clientCameraMaxZoom(cfg::ClientCameraMaxZoom, 1000.0f, "", "");
		_maxZoom = core::Var::registerVar(clientCameraMaxZoom);
		const core::VarDef clientCameraMinZoom(cfg::ClientCameraMinZoom, 0.001f, "", "");
		_minZoom = core::Var::registerVar(clientCameraMinZoom);
	}
};

TEST_F(CameraTest, testLookAt) {
	Camera camera = setup();
	EXPECT_FLOAT_EQ(math::halfPi, camera.pitch());
	EXPECT_FLOAT_EQ(0.0f, camera.yaw());
	EXPECT_FLOAT_EQ(0.0f, camera.roll());
	const math::Vec4f invecs[6] = {
		// left bottom, right bottom, right top
		math::Vec4f(-1.0f, -1.0f, 0.0f, 1.0f), math::Vec4f( 1.0f, -1.0f, 0.0f, 1.0f), math::Vec4f( 1.0f,  1.0f, 0.0f, 1.0f),
		// left bottom, right top, left top
		math::Vec4f(-1.0f, -1.0f, 0.0f, 1.0f), math::Vec4f( 1.0f,  1.0f, 0.0f, 1.0f), math::Vec4f(-1.0f,  1.0f, 0.0f, 1.0f)
	};

	math::Vec4f outvecs[6];
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

TEST_F(CameraTest, OverlaySegmentsClipBeforePerspectiveDivision) {
	Camera camera = setup(math::Vec2f(800, 600), math::Vec3f(0), math::Vec3f(0, 0, -1), math::up());
	camera.setNearPlane(1);
	camera.update(0);
	math::Vec2i a, b;
	EXPECT_FALSE(camera.worldSegmentToScreen(math::Vec3f(0, 0, 2), math::Vec3f(1, 0, 1), a, b));
	EXPECT_FALSE(camera.worldSegmentToScreen(math::Vec3f(100, 0, -5), math::Vec3f(120, 0, -5), a, b));
	ASSERT_TRUE(camera.worldSegmentToScreen(math::Vec3f(-1, 0, -5), math::Vec3f(1, 0, 0.5f), a, b));
	for (const math::Vec2i &p : {a, b}) {
		EXPECT_GE(p.x, 0);
		EXPECT_LE(p.x, 800);
		EXPECT_GE(p.y, 0);
		EXPECT_LE(p.y, 600);
	}
	ASSERT_TRUE(camera.worldSegmentToScreen(math::Vec3f(-100, 0, -5), math::Vec3f(100, 0, -5), a, b));
	EXPECT_EQ(0, a.x);
	EXPECT_EQ(800, b.x);
	EXPECT_EQ(300, a.y);
	EXPECT_EQ(300, b.y);
}

TEST_F(CameraTest, testMotion) {
	// TODO: finish this
	Camera camera = setup(math::Vec2f(1024, 768), math::Vec3f(0.0, 1.0, 0.0));
	camera.rotate(math::Vec3f(0.0f, 10.0f, 0.0f));
	camera.update(0.0);
}

TEST_F(CameraTest, testParallelLookAt) {
	Camera camera = setup(math::Vec2f(1024, 768), math::Vec3f(0.0, 10.0, 0.0), math::Vec3f(0.0), math::up());
	camera.update(0.0);
	const math::Vec4f invecs[6] = {
		// left bottom, right bottom, right top
		math::Vec4f(-1.0f, -1.0f, 0.0f, 1.0f), math::Vec4f( 1.0f, -1.0f, 0.0f, 1.0f), math::Vec4f( 1.0f,  1.0f, 0.0f, 1.0f),
		// left bottom, right top, left top
		math::Vec4f(-1.0f, -1.0f, 0.0f, 1.0f), math::Vec4f( 1.0f,  1.0f, 0.0f, 1.0f), math::Vec4f(-1.0f,  1.0f, 0.0f, 1.0f)
	};

	math::Vec4f outvecs[6];
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
	Camera camera = setup(math::Vec2f(1024, 768), math::Vec3f(0.1, 1.0, 0.1));
	camera.setMode(CameraMode::Perspective);
	camera.update(0.0);
	const math::Frustum &frustum = camera.frustum();
	EXPECT_EQ(math::FrustumResult::Inside, frustum.test(math::Vec3f(0.0, 0.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Outside, frustum.test(math::Vec3f(0.0, 1.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Intersect, frustum.test(math::Vec3f(-1.0, -1.0, -1.0), math::Vec3f(0.5, 0.5, 0.5)));
}

TEST_F(CameraTest, testMoveAndPan) {
	Camera camera = setup(math::Vec2f(1024, 768), math::Vec3f(0.0, 1.0, 0.0));
	const math::Vec3f before = camera.worldPosition();

	// moving along +z in camera-space should change world position
	const bool moved = camera.move(math::Vec3f(0.0f, 0.0f, 1.0f));
	EXPECT_TRUE(moved);
	const math::Vec3f after = camera.worldPosition();
	// For the default look-at (looking down) moving +z moves the camera up in world Y
	EXPECT_NE(before, after);

	// panning should change panOffset
	const math::Vec3f panBefore = camera.worldPosition();
	camera.pan(10, 20);
	const math::Vec3f panAfter = camera.worldPosition();
	EXPECT_NE(panBefore, panAfter);
}

TEST_F(CameraTest, testSlerp) {
	Camera camera = setup();
	const math::Quatf startQuat = camera.quaternion();
	// rotate target by 90 degrees around Y
	const math::Quatf targetQuat = math::angleAxis(math::halfPi, math::up());
	camera.slerp(targetQuat, 0.5f);
	const math::Quatf midQuat = camera.quaternion();
	// Expect quaternion to have changed but not equal to target yet
	EXPECT_FALSE(math::all(math::epsilonEqual(startQuat, midQuat, 0.0001f)));
	EXPECT_FALSE(math::all(math::epsilonEqual(targetQuat, midQuat, 0.0001f)));
}

TEST_F(CameraTest, testLerpBetweenCameras) {
	Camera a = setup(math::Vec2f(200, 200), math::Vec3f(0.0f, 10.0f, 0.0));
	Camera b = setup(math::Vec2f(200, 200), math::Vec3f(10.0f, 10.0f, 0.0));
	// ensure target is clean before lerp
	b.update(0.0);
	const math::Vec3f aBefore = a.worldPosition();
	const math::Vec3f bPos = b.worldPosition();
	a.lerp(b);
	// progress half-way
	a.update(0.5);
	const math::Vec3f aMid = a.worldPosition();
	EXPECT_FALSE(math::all(math::epsilonEqual(aBefore, aMid, 0.0001f)));
	// finish lerp
	a.update(1.0);
	const math::Vec3f aAfter = a.worldPosition();
	// after finishing lerp the world position should be (approximately) equal to target
	EXPECT_NEAR(bPos.x, aAfter.x, 0.001f);
	EXPECT_NEAR(bPos.y, aAfter.y, 0.001f);
	EXPECT_NEAR(bPos.z, aAfter.z, 0.001f);
}

TEST_F(CameraTest, testCameraFrustumCullingOrthogonal) {
	Camera camera;
	camera.setSize(math::Vec2f(100.0f, 100.0f));
	camera.setMode(CameraMode::Orthogonal);
	camera.setOrthoDepth(10.0f);
	camera.setWorldPosition(math::Vec3f(0.1, 1.0, 0.1));
	camera.lookAt(math::Vec3f(0.0), math::forward());
	camera.update(0.0);
	const math::Frustum &frustum = camera.frustum();
	EXPECT_EQ(math::FrustumResult::Inside, frustum.test(math::Vec3f(0.0, 0.0, 0.0)));
	// A point far above the camera should be outside the frustum when looking down
	EXPECT_EQ(math::FrustumResult::Outside, frustum.test(math::Vec3f(0.0, 10.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Inside, frustum.test(math::Vec3f(-1.0, -1.0, -1.0), math::Vec3f(0.5, 0.5, 0.5)));
	// TODO: add math::FrustumResult::Intersect test
}

TEST_F(CameraTest, testOrthoZoom) {
	util::ScopedVarChange zoomSpeedChange(_zoomSpeed, 0.9f);
	Camera camera;
	camera.setMode(CameraMode::Perspective);
	camera.setSize(math::Vec2i(1024, 768));
	camera.setMode(CameraMode::Orthogonal);
	camera.update(0.0);

	const float initialZoom = camera.orthoZoom();

	camera.zoom(1.0f);
	const float zoomedOut = camera.orthoZoom();
	EXPECT_GT(zoomedOut, initialZoom);
	EXPECT_LE(zoomedOut, _maxZoom->floatVal());

	const float expectedFactor = math::exp(_zoomSpeed->floatVal());
	EXPECT_NEAR(zoomedOut, initialZoom * expectedFactor, 0.001f) << "Zoom out should match configured speed";

	camera.zoom(-1.0f);
	const float zoomedIn = camera.orthoZoom();
	EXPECT_FLOAT_EQ(zoomedIn, initialZoom);

	camera.setOrthoZoom(3.5f);
	EXPECT_FLOAT_EQ(camera.orthoZoom(), 3.5f);
	camera.setOrthoZoom(0.0f);
	EXPECT_GT(camera.orthoZoom(), 0.0f);
	EXPECT_GE(zoomedIn, _minZoom->floatVal());
}

TEST_F(CameraTest, testIsometricProjectionSanitizesOrientation) {
	Camera camera;
	camera.setSize(math::Vec2i(1024, 768));
	camera.setMode(CameraMode::Isometric);
	camera.setAngles(0.1f, math::radians(30.0f), math::radians(10.0f));
	camera.update(0.0);

	EXPECT_TRUE(camera.isOrthographic());
	EXPECT_LT(camera.horizontalYaw(), 0.0f);
	EXPECT_GT(camera.horizontalYaw(), -math::halfPi);
	EXPECT_LT(camera.forward().y, -0.45f);
	EXPECT_GT(camera.forward().y, -0.75f);
}

TEST_F(CameraTest, testIsometricYawWraps) {
	Camera camera;
	camera.setSize(math::Vec2i(1024, 768));
	camera.setMode(CameraMode::Isometric);
	camera.setAngles(Camera::IsometricPitch, math::radians(179.0f), 0.0f);
	camera.update(0.0);
	const math::Vec3f before = camera.forward();

	camera.turn(math::radians(20.0f));
	camera.update(0.0);

	EXPECT_LT(camera.horizontalYaw(), 0.0f);
	EXPECT_GT(math::length(camera.forward() - before), 0.01f);
}

TEST_F(CameraTest, testIsometricTurnTwoCirclesSmallSteps) {
	Camera camera;
	camera.setSize(math::Vec2i(1024, 768));
	camera.setMode(CameraMode::Isometric);
	camera.setRotationType(CameraRotationType::Target);
	camera.setTarget(math::Vec3f(0.0f));
	camera.setTargetDistance(10.0f);
	camera.setAngles(Camera::IsometricPitch, 0.0f, 0.0f);
	camera.update(0.0);

	const math::Vec3f initialForward = camera.forward();
	const math::Vec3f initialWorldPos = camera.worldPosition();
	math::Vec3f previousForward = initialForward;
	math::Vec3f previousWorldPos = initialWorldPos;
	float accumulatedTravel = 0.0f;
	const float step = math::radians(1.0f);

	for (int i = 0; i < 720; ++i) {
		camera.turn(step);
		camera.update(1.0 / 60.0);

		const float worldDelta = math::distance(previousWorldPos, camera.worldPosition());
		const float forwardDelta = math::length(previousForward - camera.forward());
		EXPECT_GT(worldDelta, 0.01f);
		EXPECT_GT(forwardDelta, 0.001f);
		EXPECT_NEAR(10.0f, camera.targetDistance(), 0.0001f);
		accumulatedTravel += worldDelta;
		previousWorldPos = camera.worldPosition();
		previousForward = camera.forward();
	}

	EXPECT_GT(accumulatedTravel, 50.0f);
	const float finalForwardDelta = math::length(initialForward - camera.forward());
	const float finalWorldPosDelta = math::distance(initialWorldPos, camera.worldPosition());
	EXPECT_LT(finalForwardDelta, 0.01f) << "final forward delta: " << finalForwardDelta;
	EXPECT_LT(finalWorldPosDelta, 0.05f) << "final world position delta: " << finalWorldPosDelta;
}

TEST_F(CameraTest, testIsometricIdleUpdateIsStable) {
	Camera camera;
	camera.setSize(math::Vec2i(1024, 768));
	camera.setMode(CameraMode::Isometric);
	camera.setAngles(Camera::IsometricPitch, math::radians(30.0f), 0.0f);
	camera.update(0.0);

	const math::Quatf beforeQuat = camera.quaternion();
	const math::Vec3f beforeForward = camera.forward();
	const float beforeYaw = camera.horizontalYaw();

	for (int i = 0; i < 120; ++i) {
		camera.update(1.0 / 60.0);
	}

	EXPECT_TRUE(math::all(math::epsilonEqual(beforeForward, camera.forward(), 0.0001f)));
	EXPECT_TRUE(math::all(math::epsilonEqual(math::Vec4f(beforeQuat.x, beforeQuat.y, beforeQuat.z, beforeQuat.w),
										   math::Vec4f(camera.quaternion().x, camera.quaternion().y,
													 camera.quaternion().z, camera.quaternion().w),
										   0.0001f)));
	EXPECT_NEAR(beforeYaw, camera.horizontalYaw(), 0.0001f);
}

TEST_F(CameraTest, testWorldToScreen) {
	Camera camera = setup(math::Vec2f(100, 100), math::Vec3f(0.0, 0.0, 10.0), math::Vec3f(0.0, 0.0, 0.0), math::up());

	const math::Vec2i center = camera.worldToScreen(math::Vec3f(0.0, 0.0, 0.0));
	EXPECT_EQ(50, center.x);
	EXPECT_EQ(50, center.y);

	const math::Vec2i up = camera.worldToScreen(math::up());
	EXPECT_LT(up.y, center.y);
	EXPECT_EQ(up.x, center.x);

	const math::Vec2i right = camera.worldToScreen(math::right());
	EXPECT_GT(right.x, center.x);
	EXPECT_EQ(right.y, center.y);
}

TEST_F(CameraTest, testRayHonorsOffsetEdgesAndRes) {
	Camera camera = setup(math::Vec2f(800, 600), math::Vec3f(0, 0, 10), math::Vec3f(0), math::up());
	const math::Vec4f view(300, 100, 400, 300);
	math::Ray center(math::Vec3f(0), math::Vec3f(0));
	ASSERT_TRUE(camera.mouseRay(view, {500, 250}, center));
	EXPECT_NEAR(-1, center.direction.z, .001f);
	math::Ray right = center;
	ASSERT_TRUE(camera.mouseRay(view, {650, 250}, right));
	EXPECT_GT(right.direction.x, center.direction.x);
	EXPECT_FALSE(camera.mouseRay(view, {700, 250}, right));
	EXPECT_FALSE(camera.mouseRay(view, {500, 400}, right));
	EXPECT_FALSE(camera.mouseRay(view, {299, 250}, right));
	EXPECT_FALSE(camera.mouseRay(view, {500, 99}, right));
	ASSERT_TRUE(camera.mouseRay(view, {300, 100}, right));
	EXPECT_GT(right.direction.y, 0);
	camera.setSize({1600, 1200});
	camera.update(0);
	ASSERT_TRUE(camera.mouseRay(view, {500, 250}, right));
	EXPECT_NEAR(0, math::length(right.direction - center.direction), .001f);
}

TEST_F(CameraTest, testRayRejectsInvalid) {
	Camera camera = setup(math::Vec2f(800, 600), math::Vec3f(0, 0, 10), math::Vec3f(0), math::up());
	math::Ray ray(math::Vec3f(1, 2, 3), math::Vec3f(0, 0, -1));
	for (const math::Vec4f &view :
		 {math::Vec4f(0, 0, 0, 10), math::Vec4f(0, 0, 10, -1), math::Vec4f(math::NotANumber, 0, 10, 10), math::Vec4f(0, 0, math::Infinity, 10)}) {
		EXPECT_FALSE(camera.mouseRay(view, {1, 1}, ray));
	}
	EXPECT_FALSE(camera.mouseRay({0, 0, 10, 10}, {math::NotANumber, 1}, ray));
	EXPECT_FALSE(camera.mouseRay({0, 0, 10, 10}, {1, math::Infinity}, ray));
	EXPECT_EQ(math::Vec3f(1, 2, 3), ray.origin);
	EXPECT_EQ(math::Vec3f(0, 0, -1), ray.direction);
}

TEST_F(CameraTest, testMouseRay) {
	Camera camera = setup(math::Vec2f(100, 100), math::Vec3f(0.0, 0.0, 10.0), math::Vec3f(0.0, 0.0, 0.0), math::up());

	const math::Ray& ray = camera.mouseRay(math::Vec2i(50, 50));
	const math::Vec3f& dir = ray.direction;
	EXPECT_NEAR(0.0f, dir.x, 0.001f);
	EXPECT_NEAR(0.0f, dir.y, 0.001f);
	EXPECT_NEAR(-1.0f, dir.z, 0.001f);

	const math::Ray& ray2 = camera.mouseRay(math::Vec2i(50, 0));
	EXPECT_GT(ray2.direction.y, 0.0f);
	EXPECT_NEAR(0.0f, ray2.direction.x, 0.001f);

	const math::Ray& ray3 = camera.mouseRay(math::Vec2i(100, 50));
	EXPECT_GT(ray3.direction.x, 0.0f);
	EXPECT_NEAR(0.0f, ray3.direction.y, 0.001f);
}

// Regression: UPPER_LEFT + ZERO_TO_ONE must not skip UI Y flip for picking.
TEST_F(CameraTest, testMouseRayClipControlUpperLeftZeroToOne) {
	RenderState &rs = renderState();
	const bool prevOrigin = rs.clipOriginLowerLeft;
	const bool prevDepth = rs.clipDepthZeroToOne;
	rs.clipOriginLowerLeft = false;
	rs.clipDepthZeroToOne = true;

	Camera camera = setup(math::Vec2f(100, 100), math::Vec3f(0.0, 0.0, 10.0), math::Vec3f(0.0, 0.0, 0.0), math::up());

	const math::Ray &ray = camera.mouseRay(math::Vec2i(50, 50));
	EXPECT_NEAR(0.0f, ray.direction.x, 0.001f);
	EXPECT_NEAR(0.0f, ray.direction.y, 0.001f);
	EXPECT_NEAR(-1.0f, ray.direction.z, 0.001f);

	const math::Ray &rayTop = camera.mouseRay(math::Vec2i(50, 0));
	EXPECT_GT(rayTop.direction.y, 0.0f);
	EXPECT_NEAR(0.0f, rayTop.direction.x, 0.001f);

	const math::Ray &rayBottom = camera.mouseRay(math::Vec2i(50, 100));
	EXPECT_LT(rayBottom.direction.y, 0.0f);
	EXPECT_NEAR(0.0f, rayBottom.direction.x, 0.001f);

	const math::Vec2i center = camera.worldToScreen(math::Vec3f(0.0, 0.0, 0.0));
	EXPECT_EQ(50, center.x);
	EXPECT_EQ(50, center.y);
	const math::Vec2i up = camera.worldToScreen(math::up());
	EXPECT_LT(up.y, center.y);

	rs.clipOriginLowerLeft = prevOrigin;
	rs.clipDepthZeroToOne = prevDepth;
}

TEST_F(CameraTest, testSliceFrustumNearCascadesStayTight) {
	Camera camera = setup();
	camera.setNearPlane(0.1f);
	camera.setFarPlane(5000.0f);
	camera.update(0.0);

	float planes[8];
	camera.sliceFrustum(planes, 8, 4, 0.75f);
	EXPECT_FLOAT_EQ(planes[0], 0.1f);
	EXPECT_FLOAT_EQ(planes[7], 5000.0f);
	EXPECT_LT(planes[1], planes[3]);
	EXPECT_LT(planes[3], planes[5]);
	EXPECT_LT(planes[5], planes[7]);
	// Negative sliceWeight used to push the first split past 800 units on a 5000 far plane.
	EXPECT_LT(planes[1], 250.0f);
}

TEST_F(CameraTest, testOrthoClipWIsNotViewDepth) {
	Camera camera = setup();
	camera.setMode(CameraMode::Orthogonal);
	camera.setWorldPosition(math::Vec3f(0.0f, 0.0f, 200.0f));
	camera.lookAt(math::Vec3f(0.0f), math::up());
	camera.update(0.0);

	const math::Vec4f clip = camera.viewProjectionMatrix() * math::Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
	EXPECT_NEAR(clip.w, 1.0f, 0.01f);
	const float viewz = math::dot(math::Vec3f(0.0f) - camera.eye(), camera.forward());
	EXPECT_GT(viewz, 1.0f);
}

TEST_F(CameraTest, testOrthoSplitFrustumUsesSliceDepth) {
	Camera camera = setup(math::Vec2f(800, 600), math::Vec3f(0.0f, 80.0f, 80.0f), math::Vec3f(0.0f), math::up());
	camera.setMode(CameraMode::Isometric);
	camera.setOrthoDepth(10000.0f);
	camera.setNearPlane(0.1f);
	camera.update(0.0);

	math::Vec3f corners[math::FRUSTUM_VERTICES_MAX];
	camera.splitFrustum(0.1f, 50.0f, corners);
	float zmin = 1.0e9f;
	float zmax = -1.0e9f;
	for (uint8_t i = 0; i < math::FRUSTUM_VERTICES_MAX; ++i) {
		const math::Vec3f view = math::Vec3f(camera.viewMatrix() * math::Vec4f(corners[i], 1.0f));
		zmin = core_min(zmin, view.z);
		zmax = core_max(zmax, view.z);
	}
	EXPECT_LT(zmax - zmin, 400.0f);
}

TEST_F(CameraTest, testOrthoSliceFrustumPositiveDistances) {
	Camera camera = setup(math::Vec2f(800, 600), math::Vec3f(0.0f, 80.0f, 80.0f), math::Vec3f(0.0f), math::up());
	camera.setMode(CameraMode::Isometric);
	camera.setOrthoDepth(10000.0f);
	camera.setNearPlane(0.1f);
	camera.update(0.0);

	float planes[8];
	camera.sliceFrustum(planes, 8, 4, 0.75f);
	EXPECT_GT(planes[0], 0.0f);
	EXPECT_LT(planes[1], 250.0f);
	EXPECT_LT(planes[3], planes[5]);
	EXPECT_FLOAT_EQ(planes[7], 5000.0f);
	for (int i = 0; i < 8; ++i) {
		EXPECT_GT(planes[i], 0.0f);
	}
}

TEST_F(CameraTest, testBillboard) {
	Camera camera = setup(math::Vec2f(100, 100), math::Vec3f(0.0, 0.0, 10.0), math::Vec3f(0.0, 0.0, 0.0), math::up());
	math::Vec3f right, up;
	camera.billboard(&right, &up);

	EXPECT_NEAR(1.0f, right.x, 0.001f);
	EXPECT_NEAR(0.0f, right.y, 0.001f);
	EXPECT_NEAR(0.0f, right.z, 0.001f);

	EXPECT_NEAR(0.0f, up.x, 0.001f);
	EXPECT_NEAR(1.0f, up.y, 0.001f);
	EXPECT_NEAR(0.0f, up.z, 0.001f);
}

TEST_F(CameraTest, navigationPanTracksPixelsAcrossProjectionAndRotationModes) {
	const CameraMode modes[] = {CameraMode::Perspective, CameraMode::Orthogonal, CameraMode::Isometric};
	const CameraRotationType rotations[] = {CameraRotationType::Target, CameraRotationType::Eye};
	for (CameraMode mode : modes) {
		for (CameraRotationType rotation : rotations) {
			for (float distance : {50.0f, 500.0f}) {
				Camera camera;
				camera.setRotationType(CameraRotationType::Target);
				camera.setSize({1000, 800});
				camera.setTarget(math::Vec3f(0));
				camera.setWorldPosition({0, 0, distance});
				camera.setTargetDistance(distance);
				camera.setMode(mode);
				camera.update(0);
				camera.setRotationType(rotation);
				camera.update(0);
				const math::Vec2i before = camera.worldToScreen(math::Vec3f(0));
				camera.pan(40, -20);
				camera.update(0);
				const math::Vec2i after = camera.worldToScreen(math::Vec3f(0));
				EXPECT_NEAR(after.x - before.x, 40, 1) << (int)mode << "/" << (int)rotation;
				EXPECT_NEAR(after.y - before.y, -20, 1) << (int)mode << "/" << (int)rotation;
				const math::Vec3f position = camera.worldPosition();
				camera.setRotationType(CameraRotationType::Target);
				camera.update(0);
				EXPECT_LT(math::distance(position, camera.worldPosition()), 0.001f);
			}
		}
	}
}

TEST_F(CameraTest, perspectiveZoomMovesEyeWithoutChangingOrientation) {
	Camera camera;
	camera.setSize({1000, 800});
	camera.setTarget(math::Vec3f(0));
	camera.setWorldPosition({0, 0, 100});
	camera.setTargetDistance(100);
	camera.setRotationType(CameraRotationType::Eye);
	camera.update(0);
	const math::Vec3f forward = camera.forward();
	const math::Vec2i before = camera.worldToScreen({10, 0, 0});
	camera.zoom(-1);
	camera.update(0);
	const math::Vec2i after = camera.worldToScreen({10, 0, 0});
	EXPECT_GT(math::abs(after.x - 500), math::abs(before.x - 500));
	EXPECT_NEAR(camera.worldPosition().z, camera.targetDistance(), 0.001f);
	EXPECT_LT(math::distance(forward, camera.forward()), 0.001f);
	camera.zoom(1);
	camera.update(0);
	EXPECT_NEAR(camera.worldPosition().z, 100, 0.001f);
}

} // namespace video
