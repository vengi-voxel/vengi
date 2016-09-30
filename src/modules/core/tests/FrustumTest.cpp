/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/Frustum.h"

namespace core {

class FrustumTest : public core::AbstractTest {
protected:
	const float _farPlane = 500.0f;
	const float _nearPlane = 0.1f;
	Frustum _frustum;
	core::AABB<float> _aabb;
	glm::mat4 _view;
	glm::mat4 _projection;
public:
	FrustumTest() :
			_aabb(glm::vec3(0.0f), glm::vec3(1.0f)) {
	}

	void SetUp() override {
		core::AbstractTest::SetUp();
		_view = glm::lookAt(glm::vec3(0.0f), glm::right, glm::up);
		_projection = glm::perspective(glm::radians(45.0f), 0.75f, _nearPlane, _farPlane);
		updateVP(_view, _projection);
	}

	void updateVP(const glm::mat4& view, const glm::mat4& projection) {
		_frustum.updatePlanes(view, projection);
		_frustum.updateVertices(view, projection);
		_aabb = _frustum.aabb();
	}

	void updateV(const glm::mat4& view) {
		_frustum.updatePlanes(view, _projection);
		_frustum.updateVertices(view, _projection);
		_aabb = _frustum.aabb();
	}

	void updateP(const glm::mat4& projection) {
		_frustum.updatePlanes(_view, projection);
		_frustum.updateVertices(_view, projection);
		_aabb = _frustum.aabb();
	}
};

TEST_F(FrustumTest, testAABBOrtho) {
	updateVP(glm::mat4(), glm::ortho(0.0f, 50.0f, 0.0f, 100.0f, _nearPlane, _farPlane));
	ASSERT_FLOAT_EQ(_aabb.getWidthX(), 50.0f) << glm::to_string(_aabb.getLowerCorner()) << glm::to_string(_aabb.getUpperCorner());
	ASSERT_FLOAT_EQ(_aabb.getWidthY(), 100.0f) << glm::to_string(_aabb.getLowerCorner()) << glm::to_string(_aabb.getUpperCorner());
	ASSERT_NEAR(_aabb.getWidthZ(), _farPlane, _nearPlane) << glm::to_string(_aabb.getLowerCorner()) << glm::to_string(_aabb.getUpperCorner());
}

TEST_F(FrustumTest, testAABBPerspective) {
	updateV(glm::mat4());
	ASSERT_NEAR(_aabb.getWidthZ(), _farPlane, _nearPlane) << glm::to_string(_aabb.getLowerCorner()) << glm::to_string(_aabb.getUpperCorner());
}

TEST_F(FrustumTest, testCullingSphere) {
	ASSERT_FALSE(_frustum.isVisible(glm::vec3(0.0f), 0.01f));
	ASSERT_TRUE(_frustum.isVisible(glm::right * _farPlane / 2.0f + _nearPlane, 1.0f));
}

/**
 * Looking from origin to 1,0,0 (right) and checking soem aabbs to be culled
 */
TEST_F(FrustumTest, testCullingAABB) {
	{
		core::AABB<float> aabb(glm::vec3(0.0f), glm::vec3(100.0f));
		ASSERT_TRUE(_frustum.isVisible(aabb.getLowerCorner(), aabb.getUpperCorner())) << "mins(" <<
				glm::to_string(aabb.getLowerCorner()) << "), maxs(" << glm::to_string(aabb.getUpperCorner()) << ") " << " is not visible but should be: frustummins(" <<
				glm::to_string(_aabb.getLowerCorner()) << "), frustummaxs(" << glm::to_string(_aabb.getUpperCorner()) << ")";
	}
	{
		core::AABB<float> aabb(glm::vec3(-200.0f), glm::vec3(-100.0f));
		ASSERT_FALSE(_frustum.isVisible(aabb.getLowerCorner(), aabb.getUpperCorner())) << "mins(" <<
				glm::to_string(aabb.getLowerCorner()) << "), maxs(" << glm::to_string(aabb.getUpperCorner()) << ") " << " is not visible but should be: frustummins(" <<
				glm::to_string(_aabb.getLowerCorner()) << "), frustummaxs(" << glm::to_string(_aabb.getUpperCorner()) << ")";
	}
}

TEST_F(FrustumTest, testDistanceToPlane) {
	// TODO: check these values whether they are correct or not, they were just taken from the function call - so this is not really a valid test.
	// just a test that ensures that the results don't change in the future
	const float distances[FRUSTUM_PLANES_MAX] = {
			1.0f, 1.0f, 1.0f, 1.0f, 2.2004402f, -0.20044008
	};
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		const Plane& plane = _frustum[i];
		SCOPED_TRACE(core::string::format("frustum side: %u", i));
		ASSERT_FLOAT_EQ(distances[i], plane.distanceToPlane(glm::right)) << glm::to_string(glm::right) << " is not visible but should be: mins(" <<
				glm::to_string(_aabb.getLowerCorner()) << "), maxs(" << glm::to_string(_aabb.getUpperCorner()) << ")";
	}
}

/**
 * Looking from origin to 1,0,0 (right) and checking some coordinates to be culled
 */
TEST_F(FrustumTest, testCullingPoint) {
	SCOPED_TRACE(core::string::format(
			"mins(%s), maxs(%s)",
			glm::to_string(_aabb.getLowerCorner()).c_str(),
			glm::to_string(_aabb.getUpperCorner()).c_str()));
	if (_nearPlane > 0.0f) {
		ASSERT_FALSE(_aabb.containsPoint(glm::vec3(0.0f)));
		ASSERT_FALSE(_frustum.isVisible(glm::vec3(0.0f))) << "Point behind the near plane is still visible";
		ASSERT_FALSE(_aabb.containsPoint(glm::right * (_nearPlane - _nearPlane / 2.0f)));
		ASSERT_FALSE(_frustum.isVisible(glm::right * (_nearPlane - _nearPlane / 2.0f)));
	}
	ASSERT_TRUE(_aabb.containsPoint(glm::right));
	ASSERT_TRUE(_frustum.isVisible(glm::right));
	ASSERT_TRUE(_aabb.containsPoint(glm::right * _nearPlane)) << glm::to_string(glm::right) << " is not visible but should be";
	ASSERT_TRUE(_frustum.isVisible(glm::right * _nearPlane)) << glm::to_string(glm::right * _nearPlane) << " is not visible but should be";
	ASSERT_FALSE(_aabb.containsPoint(glm::up));
	ASSERT_FALSE(_frustum.isVisible(glm::up));
	ASSERT_FALSE(_aabb.containsPoint(glm::down));
	ASSERT_FALSE(_frustum.isVisible(glm::down));
	ASSERT_FALSE(_aabb.containsPoint(glm::forward));
	ASSERT_FALSE(_frustum.isVisible(glm::forward));
	ASSERT_FALSE(_aabb.containsPoint(glm::backward));
	ASSERT_FALSE(_frustum.isVisible(glm::backward));
	ASSERT_FALSE(_aabb.containsPoint(glm::left));
	ASSERT_FALSE(_frustum.isVisible(glm::left));
	ASSERT_FALSE(_aabb.containsPoint(glm::right * _farPlane + 1.0f));
	ASSERT_FALSE(_frustum.isVisible(glm::right * _farPlane + 1.0f)) << glm::to_string(glm::right * _farPlane + 1.0f) << " should be culled because it's outside the frustum";
}

}
