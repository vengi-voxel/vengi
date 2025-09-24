/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "core/GLM.h"
#include "core/StringUtil.h"
#include "math/Frustum.h"
#include "video/Camera.h"

namespace video {

class CameraTest : public app::AbstractTest {
public:
	virtual ~CameraTest() {}
protected:
	// looking straight down is the default
	Camera setup(const glm::vec2& dimension = glm::vec2(1024, 768), const glm::vec3& position = glm::vec3(0.0, 1.0, 0.0), const glm::vec3& lookAt = glm::vec3(0.0), const glm::vec3& lookAlong = glm::forward()) {
		Camera camera;
		camera.setNearPlane(0.1f);
		camera.setFarPlane(100.0f);
		camera.setSize(dimension);
		camera.setWorldPosition(position);
		camera.lookAt(lookAt, lookAlong);
		camera.update(0.0);
		return camera;
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
	const math::Frustum& frustum = camera.frustum();
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
	const glm::vec3 panBefore = camera.panOffset();
	camera.pan(10, 20);
	const glm::vec3 panAfter = camera.panOffset();
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

TEST_F(CameraTest, DISABLED_testCameraFrustumCullingOrthogonal) {
	Camera camera;
	camera.setSize(glm::vec2(100.0f, 100.0f));
	camera.setMode(CameraMode::Orthogonal);
	camera.setWorldPosition(glm::vec3(0.1, 1.0, 0.1));
	camera.lookAt(glm::vec3(0.0), glm::forward());
	camera.update(0.0);
	const math::Frustum& frustum = camera.frustum();
	EXPECT_EQ(math::FrustumResult::Inside, frustum.test(glm::vec3(0.0, 0.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Outside, frustum.test(glm::vec3(0.0, 1.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Intersect, frustum.test(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(0.5, 0.5, 0.5)));
}

}
