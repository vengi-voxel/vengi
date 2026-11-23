/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "math/Frustum.h"
#include "core/StringUtil.h"
#include "math/AABB.h"
#include "math/Mat4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Mat4Ops.h"
#include "math/Axes.h"
#include "math/Scalar.h"

namespace math {

class FrustumTest : public app::AbstractTest {
protected:
	const float _farPlane = 500.0f;
	const float _nearPlane = 0.1f;
	Frustum _frustum;
	math::AABB<float> _aabb;
	math::Mat4f _view = math::Mat4f(1.0f);
	math::Mat4f _projection = math::Mat4f(1.0f);
public:
	FrustumTest() :
			_aabb(math::Vec3f(0.0f), math::Vec3f(1.0f)) {
	}

	const char *toString(const math::Vec3f &v) {
		static char buf[128];
		core::String::formatBuf(buf, sizeof(buf), "%f:%f:%f", v.x, v.y, v.z);
		return buf;
	}

	void SetUp() override {
		app::AbstractTest::SetUp();
		/* Looking from origin to 1,0,0 (right) */
		_view = math::lookAt(math::Vec3f(0.0f), math::right(), math::up());
		_projection = math::perspective(math::radians(45.0f), 0.75f, _nearPlane, _farPlane);
		updateVP(_view, _projection);
	}

	void updateVP(const math::Mat4f& view, const math::Mat4f& projection) {
		_frustum.update(view, projection);
		_aabb = _frustum.aabb();
	}

	void updateV(const math::Mat4f& view) {
		_frustum.update(view, _projection);
		_aabb = _frustum.aabb();
	}

	void updateP(const math::Mat4f& projection) {
		_frustum.update(_view, projection);
		_aabb = _frustum.aabb();
	}
};

TEST_F(FrustumTest, testAABBOrtho) {
	updateVP(math::Mat4f(1.0f), math::ortho(0.0f, 50.0f, 0.0f, 100.0f, _nearPlane, -_farPlane));
	EXPECT_FLOAT_EQ(_aabb.getWidthX(), 50.0f) << toString(_aabb.getLowerCorner()) << toString(_aabb.getUpperCorner());
	EXPECT_FLOAT_EQ(_aabb.getWidthY(), 100.0f) << toString(_aabb.getLowerCorner()) << toString(_aabb.getUpperCorner());
	EXPECT_NEAR(_aabb.getWidthZ(), _farPlane, _nearPlane) << toString(_aabb.getLowerCorner()) << toString(_aabb.getUpperCorner());
	EXPECT_TRUE(_frustum.isVisible(math::Vec3f(1.0f), 1.0f));
	EXPECT_EQ(FrustumResult::Inside, _frustum.test(math::Vec3f(1.0f), math::Vec3f(2.0f)));
	EXPECT_TRUE(_frustum.isVisible(math::Vec3f(1.0f), math::Vec3f(2.0f)));
	EXPECT_TRUE(_frustum.isVisible(math::Vec3f(48.0f), 1.0f));
	EXPECT_EQ(FrustumResult::Inside, _frustum.test(math::Vec3f(48.0f), math::Vec3f(50.0f)));
	EXPECT_TRUE(_frustum.isVisible(math::Vec3f(48.0f), math::Vec3f(50.0f)));
}

TEST_F(FrustumTest, testAABBPerspective) {
	updateV(math::Mat4f(1.0f));
	EXPECT_NEAR(_aabb.getWidthZ(), _farPlane, _nearPlane) << toString(_aabb.getLowerCorner()) << toString(_aabb.getUpperCorner());
}

TEST_F(FrustumTest, testCullingSphere) {
	EXPECT_FALSE(_frustum.isVisible(math::Vec3f(0.0f), 0.01f));
	EXPECT_TRUE(_frustum.isVisible(math::right() * _farPlane / 2.0f + _nearPlane, 1.0f));
}

TEST_F(FrustumTest, testCullingAABBPositive) {
	const math::AABB<float> aabb(math::Vec3f(0.0f), math::Vec3f(100.0f));
	EXPECT_TRUE(_frustum.isVisible(aabb.getLowerCorner(), aabb.getUpperCorner())) << "AABB is not visible but should be";
}

TEST_F(FrustumTest, testCullingAABBNegative) {
	const math::AABB<float> aabb(math::Vec3f(-200.0f), math::Vec3f(-100.0f));
	EXPECT_FALSE(_frustum.isVisible(aabb.getLowerCorner(), aabb.getUpperCorner())) << "AABB is not visible but should be";
}

TEST_F(FrustumTest, testInsideOutsidePoint) {
	EXPECT_EQ(math::FrustumResult::Inside, _frustum.test(math::Vec3f(_nearPlane, 0.0, 0.0)));
	EXPECT_EQ(math::FrustumResult::Outside, _frustum.test(math::Vec3f(0.0, 0.0, 0.0)));
}

TEST_F(FrustumTest, testIntersectionInsideOutsideAABB) {
	EXPECT_EQ(math::FrustumResult::Inside, _frustum.test(math::Vec3f(_farPlane / 2.0f - 0.5f, -0.5, -0.5), math::Vec3f(_farPlane / 2.0f + 0.5f, 0.5, 0.5)));
	EXPECT_EQ(math::FrustumResult::Outside, _frustum.test(math::Vec3f(-1.0, -1.0, -1.0), math::Vec3f(-0.5, -0.5, -0.5)));
	EXPECT_EQ(math::FrustumResult::Intersect, _frustum.test(math::Vec3f(-1.0, -1.0, -1.0), math::Vec3f(0.5, 0.5, 0.5)));
}

TEST_F(FrustumTest, testCullingPoint) {
	if (_nearPlane > 0.0f) {
		EXPECT_FALSE(_aabb.containsPoint(math::Vec3f(0.0f)));
		EXPECT_FALSE(_frustum.isVisible(math::Vec3f(0.0f))) << "Point behind the near plane is still visible";
		EXPECT_FALSE(_aabb.containsPoint(math::right() * (_nearPlane - _nearPlane / 2.0f)));
		EXPECT_FALSE(_frustum.isVisible(math::right() * (_nearPlane - _nearPlane / 2.0f)));
	}
	EXPECT_TRUE(_aabb.containsPoint(math::right()));
	EXPECT_TRUE(_frustum.isVisible(math::right()));
	EXPECT_TRUE(_aabb.containsPoint(math::right() * _nearPlane)) << toString(math::right()) << " is not visible but should be";
	EXPECT_TRUE(_frustum.isVisible(math::right() * _nearPlane)) << toString(math::right() * _nearPlane) << " is not visible but should be";
	EXPECT_FALSE(_aabb.containsPoint(math::up()));
	EXPECT_FALSE(_frustum.isVisible(math::up()));
	EXPECT_FALSE(_aabb.containsPoint(math::down()));
	EXPECT_FALSE(_frustum.isVisible(math::down()));
	EXPECT_FALSE(_aabb.containsPoint(math::forward()));
	EXPECT_FALSE(_frustum.isVisible(math::forward()));
	EXPECT_FALSE(_aabb.containsPoint(math::backward()));
	EXPECT_FALSE(_frustum.isVisible(math::backward()));
	EXPECT_FALSE(_aabb.containsPoint(math::left()));
	EXPECT_FALSE(_frustum.isVisible(math::left()));
	EXPECT_FALSE(_aabb.containsPoint(math::right() * _farPlane + 1.0f));
	EXPECT_FALSE(_frustum.isVisible(math::right() * _farPlane + 1.0f)) << toString(math::right() * _farPlane + 1.0f) << " should be culled because it's outside the frustum";
}

TEST_F(FrustumTest, testStaticFrustumCheck) {
	EXPECT_TRUE(Frustum::isVisible(math::Vec3f(0.0, 0.0, 0.0f), math::radians(45.0f), math::Vec3f(1.0f, 0.0f, 1.0f), math::radians(10.0f)));
	EXPECT_FALSE(Frustum::isVisible(math::Vec3f(0.0, 0.0, 0.0f), math::radians(45.0f), math::Vec3f(-1.0f, 0.0f, 1.0f), math::radians(10.0f)));
	EXPECT_FALSE(Frustum::isVisible(math::Vec3f(0.0, 0.0, 0.0f), math::radians(45.0f), math::Vec3f(0.0f, 0.0f, 1.0f), math::radians(10.0f)));
	EXPECT_FALSE(Frustum::isVisible(math::Vec3f(0.0, 0.0, 0.0f), math::radians(45.0f), math::Vec3f(1.0f, 0.0f, 0.0f), math::radians(10.0f)));
}

TEST_F(FrustumTest, testOrthoFrustum) {
	const math::Vec3i mins(-64, -32, -32);
	const math::Vec3i maxs(64, 32, 96);
	math::Frustum frustum(mins, maxs);
	EXPECT_EQ(frustum.aabb(), math::AABB<float>(mins, maxs));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(-64, -32, 64), math::Vec3i(-32, 0, 96)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(-64, 0, 64), math::Vec3i(-32, 32, 96)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(-32, -32, 64), math::Vec3i(0, 0, 96)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(-32, 0, 64), math::Vec3i(0, 32, 96)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(0, -32, 64), math::Vec3i(32, 0, 96)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(0, 0, 64), math::Vec3i(32, 32, 96)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(32, -32, -32), math::Vec3i(64, 0, 0)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(32, -32, 0), math::Vec3i(64, 0, 32)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(32, -32, 32), math::Vec3i(64, 0, 64)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(32, -32, 64), math::Vec3i(64, 0, 96)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(32, 0, -32), math::Vec3i(64, 32, 0)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(32, 0, 0), math::Vec3i(64, 32, 32)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(32, 0, 32), math::Vec3i(64, 32, 64)));
	EXPECT_TRUE(frustum.isVisible(math::Vec3i(32, 0, 64), math::Vec3i(64, 32, 96)));
	EXPECT_FALSE(frustum.isVisible(math::Vec3i(-66, -32, 64), math::Vec3i(-65, 0, 96)));
}

}
