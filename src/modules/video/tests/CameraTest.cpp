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
	camera.lookAt(glm::vec3(0.0));
	camera.update(0l);
	// looking straight down
	EXPECT_FLOAT_EQ(-glm::half_pi<float>(), camera.pitch());
	EXPECT_FLOAT_EQ(0.0f, camera.yaw());
	EXPECT_FLOAT_EQ(0.0f, camera.roll());
}

TEST_F(CameraTest, testMotion) {
	Camera camera;
	camera.setPosition(glm::vec3(0.0, 1.0, 0.0));
	camera.lookAt(glm::vec3(0.0));
	camera.rotate(10, 0, 1.0f);
	camera.update(0l);
}

}
