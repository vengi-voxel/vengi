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

TEST_F(CameraTest, testCullingPerspective) {
	Camera camera;
	camera.setMode(CameraMode::Perspective);
	camera.setPosition(glm::vec3(0.0, 1.0, 0.0));
	camera.lookAt(glm::vec3(0.0), glm::forward);
	camera.update(0l);
	ASSERT_EQ(video::FrustumResult::Outside, camera.testFrustum(glm::vec3(0.0, 1.0, 0.0)));
	ASSERT_EQ(video::FrustumResult::Inside, camera.testFrustum(glm::vec3(0.0, 0.0, 0.0)));
	ASSERT_EQ(video::FrustumResult::Intersect, camera.testFrustum(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(0.5, 0.5, 0.5)));
}

TEST_F(CameraTest, testCullingOrthogonal) {
	Camera camera;
	camera.setMode(CameraMode::Orthogonal);
	camera.setPosition(glm::vec3(0.0, 1.0, 0.0));
	camera.lookAt(glm::vec3(0.0), glm::forward);
	camera.update(0l);
	ASSERT_EQ(video::FrustumResult::Outside, camera.testFrustum(glm::vec3(0.0, 1.0, 0.0)));
	ASSERT_EQ(video::FrustumResult::Inside, camera.testFrustum(glm::vec3(0.0, 0.0, 0.0)));
	ASSERT_EQ(video::FrustumResult::Intersect, camera.testFrustum(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(0.5, 0.5, 0.5)));
}

}
