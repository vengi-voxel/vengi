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
		/* Looking from origin to 1,0,0 (right) */
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

TEST_F(FrustumTest, testCullingAABBPositive) {
	const core::AABB<float> aabb(glm::vec3(0.0f), glm::vec3(100.0f));
	SCOPED_TRACE(core::string::format("mins(%s), maxs(%s), frustummins(%s), frustummaxs(%s)",
			glm::to_string(aabb.getLowerCorner()).c_str(),
			glm::to_string(aabb.getUpperCorner()).c_str(),
			glm::to_string(_aabb.getLowerCorner()).c_str(),
			glm::to_string(_aabb.getUpperCorner()).c_str()));
	ASSERT_TRUE(_frustum.isVisible(aabb.getLowerCorner(), aabb.getUpperCorner())) << "AABB is not visible but should be";
}

TEST_F(FrustumTest, testCullingAABBNegative) {
	const core::AABB<float> aabb(glm::vec3(-200.0f), glm::vec3(-100.0f));
	SCOPED_TRACE(core::string::format("mins(%s), maxs(%s), frustummins(%s), frustummaxs(%s)",
			glm::to_string(aabb.getLowerCorner()).c_str(),
			glm::to_string(aabb.getUpperCorner()).c_str(),
			glm::to_string(_aabb.getLowerCorner()).c_str(),
			glm::to_string(_aabb.getUpperCorner()).c_str()));
	ASSERT_FALSE(_frustum.isVisible(aabb.getLowerCorner(), aabb.getUpperCorner())) << "AABB is not visible but should be";
}

TEST_F(FrustumTest, testInsideOutsidePoint) {
	ASSERT_EQ(core::FrustumResult::Inside, _frustum.test(glm::vec3(_nearPlane, 0.0, 0.0)));
	ASSERT_EQ(core::FrustumResult::Outside, _frustum.test(glm::vec3(0.0, 0.0, 0.0)));
}

TEST_F(FrustumTest, testIntersectionInsideOutsideAABB) {
	ASSERT_EQ(core::FrustumResult::Inside, _frustum.test(glm::vec3(_farPlane / 2.0f - 0.5f, -0.5, -0.5), glm::vec3(_farPlane / 2.0f + 0.5f, 0.5, 0.5)));
	ASSERT_EQ(core::FrustumResult::Outside, _frustum.test(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(-0.5, -0.5, -0.5)));
	ASSERT_EQ(core::FrustumResult::Intersect, _frustum.test(glm::vec3(-1.0, -1.0, -1.0), glm::vec3(0.5, 0.5, 0.5)));
}

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

TEST_F(FrustumTest, testStaticFrustumCheck) {
	ASSERT_TRUE(Frustum::isVisible(glm::vec3(0.0, 0.0, 0.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 1.0f), glm::radians(10.0f)));
	ASSERT_FALSE(Frustum::isVisible(glm::vec3(0.0, 0.0, 0.0f), glm::radians(45.0f), glm::vec3(-1.0f, 0.0f, 1.0f), glm::radians(10.0f)));
	ASSERT_FALSE(Frustum::isVisible(glm::vec3(0.0, 0.0, 0.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::radians(10.0f)));
	ASSERT_FALSE(Frustum::isVisible(glm::vec3(0.0, 0.0, 0.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(10.0f)));
}


}
