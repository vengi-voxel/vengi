/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "video/Camera.h"

namespace video {

class CameraTest : public core::AbstractTest {
protected:
	// looking straight down is the default
	Camera setup(const glm::vec2& dimension = glm::vec2(1024, 768), const glm::vec3& position = glm::vec3(0.0, 1.0, 0.0), const glm::vec3& lookAt = glm::vec3(0.0), const glm::vec3& lookAlong = glm::forward) {
		Camera camera;
		camera.setNearPlane(0.1f);
		camera.setFarPlane(100.0f);
		camera.init(dimension);
		camera.setPosition(position);
		camera.lookAt(lookAt, lookAlong);
		camera.update(0l);
		return camera;
	}
};

::std::ostream& operator<<(::std::ostream& ostream, const Ray& val) {
	return ostream << "origin: " << glm::to_string(val.origin) << " - direction: " << glm::to_string(val.direction);
}

TEST_F(CameraTest, testLookAt) {
	Camera camera = setup();
	EXPECT_FLOAT_EQ(glm::half_pi<float>(), camera.pitch());
	EXPECT_FLOAT_EQ(0.0f, camera.yaw());
	EXPECT_FLOAT_EQ(0.0f, camera.roll());
}

TEST_F(CameraTest, testScreenRayStraightDown) {
	Camera camera = setup();
	// get the world position from the center of the screen
	const Ray& ray = camera.screenRay(glm::vec2(0.5f));
	ASSERT_TRUE(glm::all(glm::epsilonEqual(glm::down, ray.direction, 0.00001f))) << ray << " - " << ray.direction.x << ", " << ray.direction.y << ", " << ray.direction.z;
	ASSERT_TRUE(glm::all(glm::epsilonEqual(camera.position(), ray.origin, 0.00001f))) << ray << " - " << ray.origin.x << ", " << ray.origin.y << ", " << ray.origin.z;
}

TEST_F(CameraTest, testMotion) {
	// TODO: finish this
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
	camera.init(glm::vec2(100.0f, 100.0f));
	camera.setMode(CameraMode::Orthogonal);
	camera.setPosition(glm::vec3(0.1, 1.0, 0.1));
	camera.lookAt(glm::vec3(0.0), glm::forward);
	camera.update(0l);
	const core::Frustum& frustum = camera.frustum();
	SCOPED_TRACE(core::string::format("mins(%s), maxs(%s), frustummins(%s), frustummaxs(%s)",
			glm::to_string(camera.aabb().getLowerCorner()).c_str(),
			glm::to_string(camera.aabb().getUpperCorner()).c_str(),
			glm::to_string(frustum.aabb().getLowerCorner()).c_str(),
			glm::to_string(frustum.aabb().getUpperCorner()).c_str()));
	ASSERT_EQ(core::FrustumResult::Inside, frustum.test(glm::vec3(0.0, 0.0, 0.0)));
	ASSERT_EQ(core::FrustumResult::Outside, frustum.test(glm::vec3(0.0, 1.0, 0.0)));
	ASSERT_EQ(core::FrustumResult::Intersect, frustum.test(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(0.5, 0.5, 0.5)));
}

}
