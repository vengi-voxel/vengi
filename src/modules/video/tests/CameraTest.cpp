/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/GLM.h"
#include "core/StringUtil.h"
#include "math/Frustum.h"
#include "math/AABB.h"
#include "video/Camera.h"
#include "video/Ray.h"

namespace video {

class CameraTest : public core::AbstractTest {
public:
	virtual ~CameraTest() {}
protected:
	// looking straight down is the default
	Camera setup(const glm::vec2& dimension = glm::vec2(1024, 768), const glm::vec3& position = glm::vec3(0.0, 1.0, 0.0), const glm::vec3& lookAt = glm::vec3(0.0), const glm::vec3& lookAlong = glm::forward) {
		Camera camera;
		camera.setNearPlane(0.1f);
		camera.setFarPlane(100.0f);
		camera.init(glm::ivec2(0), dimension, dimension);
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

TEST_F(CameraTest, testScreenRayStraightDown) {
	Camera camera = setup();
	// get the world position from the center of the screen
	const Ray& ray = camera.screenRay(glm::vec2(0.5f));
	EXPECT_TRUE(glm::all(glm::epsilonEqual(glm::down, ray.direction, 0.00001f))) << ray << " - " << ray.direction.x << ", " << ray.direction.y << ", " << ray.direction.z;
	EXPECT_TRUE(glm::all(glm::epsilonEqual(camera.position(), ray.origin, 0.00001f))) << ray << " - " << ray.origin.x << ", " << ray.origin.y << ", " << ray.origin.z;
}

TEST_F(CameraTest, testMotion) {
	// TODO: finish this
	Camera camera;
	camera.setPosition(glm::vec3(0.0, 1.0, 0.0));
	camera.lookAt(glm::vec3(0.0), glm::forward);
	camera.rotate(glm::vec3(0.0f, 10.0f, 0.0f));
	camera.update(0l);
}

TEST_F(CameraTest, testParallelLookAt) {
	Camera camera;
	camera.setPosition(glm::vec3(0.0, 10.0, 0.0));
	camera.lookAt(glm::vec3(0.0), glm::up);
	camera.update(0l);
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
	Camera camera;
	camera.setMode(CameraMode::Perspective);
	camera.setPosition(glm::vec3(0.1, 1.0, 0.1));
	camera.lookAt(glm::vec3(0.0), glm::forward);
	camera.update(0l);
	const math::Frustum& frustum = camera.frustum();
	EXPECT_EQ(math::FrustumResult::Inside, frustum.test(glm::vec3(0.0, 0.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Outside, frustum.test(glm::vec3(0.0, 1.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Intersect, frustum.test(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(0.5, 0.5, 0.5)));
}

TEST_F(CameraTest, testCameraFrustumCullingOrthogonal) {
	Camera camera;
	camera.init(glm::ivec2(0), glm::vec2(100.0f, 100.0f), glm::vec2(100.0f, 100.0f));
	camera.setMode(CameraMode::Orthogonal);
	camera.setPosition(glm::vec3(0.1, 1.0, 0.1));
	camera.lookAt(glm::vec3(0.0), glm::forward);
	camera.update(0l);
	const math::Frustum& frustum = camera.frustum();
	SCOPED_TRACE(core::string::format("mins(%s), maxs(%s), frustummins(%s), frustummaxs(%s)",
			glm::to_string(camera.aabb().getLowerCorner()).c_str(),
			glm::to_string(camera.aabb().getUpperCorner()).c_str(),
			glm::to_string(frustum.aabb().getLowerCorner()).c_str(),
			glm::to_string(frustum.aabb().getUpperCorner()).c_str()));
	EXPECT_EQ(math::FrustumResult::Inside, frustum.test(glm::vec3(0.0, 0.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Outside, frustum.test(glm::vec3(0.0, 1.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Intersect, frustum.test(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(0.5, 0.5, 0.5)));
}

}
