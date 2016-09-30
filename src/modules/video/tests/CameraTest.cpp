/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "video/Camera.h"

namespace video {

class CameraTest : public core::AbstractTest {
};

TEST_F(CameraTest, testLookAt) {
	Camera camera;
	camera.setPosition(glm::vec3(0.0, 1.0, 0.0));
	camera.lookAt(glm::vec3(0.0), glm::forward);
	camera.update(0l);
	// looking straight down
	EXPECT_FLOAT_EQ(glm::half_pi<float>(), camera.pitch());
	EXPECT_FLOAT_EQ(0.0f, camera.yaw());
	EXPECT_FLOAT_EQ(0.0f, camera.roll());
}

TEST_F(CameraTest, testMotion) {
	Camera camera;
	camera.setPosition(glm::vec3(0.0, 1.0, 0.0));
	camera.lookAt(glm::vec3(0.0), glm::forward);
	camera.rotate(glm::vec3(0.0f, 10.0f, 0.0f));
	camera.update(0l);
}

TEST_F(CameraTest, testCameraFrustumCullingPerspective) {
	Camera camera;
	camera.setMode(CameraMode::Perspective);
	camera.setPosition(glm::vec3(0.1, 1.0, 0.1));
	camera.lookAt(glm::vec3(0.0), glm::forward);
	camera.update(0l);
	const core::Frustum& frustum = camera.frustum();
	ASSERT_EQ(core::FrustumResult::Inside, frustum.test(glm::vec3(0.0, 0.0, 0.0)));
	ASSERT_EQ(core::FrustumResult::Outside, frustum.test(glm::vec3(0.0, 1.0, 0.0)));
	ASSERT_EQ(core::FrustumResult::Intersect, frustum.test(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(0.5, 0.5, 0.5)));
}

TEST_F(CameraTest, testCameraFrustumCullingOrthogonal) {
	Camera camera;
	camera.setMode(CameraMode::Orthogonal);
	camera.setPosition(glm::vec3(0.1, 1.0, 0.1));
	camera.lookAt(glm::vec3(0.0), glm::forward);
	camera.update(0l);
	const core::Frustum& frustum = camera.frustum();
	ASSERT_EQ(core::FrustumResult::Inside, frustum.test(glm::vec3(0.0, 0.1, 0.0)));
	ASSERT_EQ(core::FrustumResult::Outside, frustum.test(glm::vec3(0.0, 1.1, 0.0)));
	ASSERT_EQ(core::FrustumResult::Intersect, frustum.test(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(0.5, 0.5, 0.5)));
}

}
