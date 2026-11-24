/**
 * @file
 */

#include "math/Functions.h"
#include "math/tests/TestMathHelper.h"
#include "app/tests/AbstractTest.h"
#include "math/Mat4f.h"
#include "math/Mat3f.h"
#include "math/Mat3d.h"
#include "math/Mat2f.h"
#include "math/Mat3x4f.h"
#include "math/Mat4x3f.h"
#include "math/Quatf.h"
#include "math/Vec2f.h"
#include "math/Vec2d.h"
#include "math/Vec2i.h"
#include "math/Vec2u.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec3d.h"
#include "math/Vec3u.h"
#include "math/Vec4f.h"
#include "math/Vec4d.h"
#include "math/Vec4i.h"
#include "math/Vec4u.h"
#include "math/AlignedVec4f.h"
#include "math/AlignedVec4i.h"
#include "math/U8Vec3.h"
#include "math/U8Vec4.h"
#include "math/U16Vec3.h"
#include "math/BVec2.h"
#include "math/BVec3.h"
#include "math/BVec4.h"
#include "math/Mat4Ops.h"
#include "math/Scalar.h"
#include <type_traits>

static_assert(std::is_trivial<math::Vec2f>::value && std::is_trivial<math::Vec2d>::value && std::is_trivial<math::Vec2i>::value &&
			  std::is_trivial<math::Vec2u>::value, "vec2");
static_assert(std::is_trivial<math::Vec3f>::value && std::is_trivial<math::Vec3d>::value && std::is_trivial<math::Vec3i>::value &&
			  std::is_trivial<math::Vec3u>::value, "vec3");
static_assert(std::is_trivial<math::Vec4f>::value && std::is_trivial<math::Vec4d>::value && std::is_trivial<math::Vec4i>::value &&
			  std::is_trivial<math::Vec4u>::value, "vec4");
static_assert(std::is_trivial<math::AlignedVec4f>::value && std::is_trivial<math::AlignedVec4i>::value, "aligned vec4");
static_assert(std::is_trivial<math::U8Vec3>::value && std::is_trivial<math::U8Vec4>::value && std::is_trivial<math::U16Vec3>::value, "packed");
static_assert(std::is_trivial<math::BVec2>::value && std::is_trivial<math::BVec3>::value && std::is_trivial<math::BVec4>::value, "bvec");
static_assert(std::is_trivial<math::Mat2f>::value && std::is_trivial<math::Mat3f>::value && std::is_trivial<math::Mat3d>::value &&
			  std::is_trivial<math::Mat4f>::value && std::is_trivial<math::Mat3x4f>::value && std::is_trivial<math::Mat4x3f>::value, "mat");
static_assert(std::is_trivial<math::Quatf>::value, "quat");

static constexpr math::Vec4f kZero = math::Vec4f();
static_assert(kZero.x == 0.0f && kZero.y == 0.0f && kZero.z == 0.0f && kZero.w == 0.0f, "value init is zero");
static constexpr math::Vec3i kRing[] = {math::Vec3i(), math::Vec3i(1, 0, 0)};
static_assert(kRing[0].x == 0 && kRing[1].x == 1, "constexpr vec3i");
static constexpr math::Mat4f kIdentity = math::Mat4f::identity();
static_assert(kIdentity[0][0] == 1.0f && kIdentity[1][1] == 1.0f && kIdentity[2][2] == 1.0f && kIdentity[3][3] == 1.0f &&
			  kIdentity[0][1] == 0.0f, "mat4 identity");
static constexpr math::Quatf kQuatIdentity = math::Quatf::identity();
static_assert(kQuatIdentity.x == 0.0f && kQuatIdentity.y == 0.0f && kQuatIdentity.z == 0.0f && kQuatIdentity.w == 1.0f, "quat identity");

TEST(MathTest, testTransform) {
	const math::Mat4f &mat = math::eulerAngleY(math::radians(90.0f));
	const math::Vec3i expected(0, 0, 1);
	const math::Vec3i &destination = math::transform(mat, math::Vec3i(0), math::Vec3f(0.0f));
	EXPECT_EQ(expected, destination) << expected << " vs " << destination;
}

TEST(MathTest, testTransformVec3) {
	const math::Mat4f &mat = math::eulerAngleY(math::radians(90.0f));
	const math::Vec3f expected(0.0f, 0.0f, 0.0f);
	const math::Vec3f &destination = math::transform(mat, math::Vec3f(0.0f), math::Vec3f(0.0f));
	EXPECT_NEAR(expected.x, destination.x, 0.0001f);
	EXPECT_NEAR(expected.y, destination.y, 0.0001f);
	EXPECT_NEAR(expected.z, destination.z, 0.0001f);
}

TEST(MathTest, testMat3x4Affine) {
	const math::Mat3x4f translation(math::Vec4f(1.0f, 0.0f, 0.0f, 2.0f), math::Vec4f(0.0f, 1.0f, 0.0f, 0.0f),
								   math::Vec4f(0.0f, 0.0f, 1.0f, 0.0f));
	const math::Vec3f moved = math::transform(translation, math::Vec3f(1.0f, 0.0f, 0.0f));
	EXPECT_NEAR(moved.x, 3.0f, 0.0001f);
	EXPECT_NEAR(moved.y, 0.0f, 0.0001f);
	EXPECT_NEAR(moved.z, 0.0f, 0.0001f);

	// 90 degrees about Z: (x, y) -> (-y, x)
	const math::Mat3x4f rotation(math::Vec4f(0.0f, 1.0f, 0.0f, 0.0f), math::Vec4f(-1.0f, 0.0f, 0.0f, 0.0f),
								 math::Vec4f(0.0f, 0.0f, 1.0f, 0.0f));
	const math::Vec3f spun = math::transform(rotation * translation, math::Vec3f(1.0f, 0.0f, 0.0f));
	EXPECT_NEAR(spun.x, 0.0f, 0.0001f);
	EXPECT_NEAR(spun.y, 3.0f, 0.0001f);
	EXPECT_NEAR(spun.z, 0.0f, 0.0001f);
}

TEST(MathTest, testFloorCeilToInt) {
	EXPECT_EQ(2, math::floorToInt(2.0f));
	EXPECT_EQ(2, math::ceilToInt(2.0f));
	EXPECT_EQ(2, math::floorToInt(2.3f));
	EXPECT_EQ(3, math::ceilToInt(2.3f));
	EXPECT_EQ(-2, math::floorToInt(-2.0f));
	EXPECT_EQ(-2, math::ceilToInt(-2.0f));
	EXPECT_EQ(-3, math::floorToInt(-2.3f));
	EXPECT_EQ(-2, math::ceilToInt(-2.3f));
	EXPECT_EQ(0, math::floorToInt(0.0f));
	EXPECT_EQ(0, math::ceilToInt(0.0f));
	EXPECT_EQ(-1, math::floorToInt(-0.1f));
	EXPECT_EQ(0, math::ceilToInt(-0.1f));
}

TEST(MathTest, testRotateVec3) {
	const math::Vec3f x = math::rotateZ(math::Vec3f(1.0f, 0.0f, 0.0f), math::halfPi);
	EXPECT_NEAR(0.0f, x.x, 0.0001f);
	EXPECT_NEAR(1.0f, x.y, 0.0001f);
	EXPECT_NEAR(0.0f, x.z, 0.0001f);
	const math::Vec3f y = math::rotateX(math::Vec3f(0.0f, 1.0f, 0.0f), math::halfPi);
	EXPECT_NEAR(0.0f, y.x, 0.0001f);
	EXPECT_NEAR(0.0f, y.y, 0.0001f);
	EXPECT_NEAR(1.0f, y.z, 0.0001f);
	const math::Vec3f z = math::rotateY(math::Vec3f(0.0f, 0.0f, 1.0f), math::halfPi);
	EXPECT_NEAR(1.0f, z.x, 0.0001f);
	EXPECT_NEAR(0.0f, z.y, 0.0001f);
	EXPECT_NEAR(0.0f, z.z, 0.0001f);
}

TEST(MathTest, testVecMod) {
	EXPECT_EQ(math::Vec3i(1, 2, 0), math::Vec3i(5, 6, 4) % 4);
	EXPECT_EQ(math::Vec3i(1, 0, 1), math::Vec3i(5, 6, 7) % math::Vec3i(4, 3, 2));
	constexpr math::Vec3i ring[] = {{1, 0, 0}, {-1, 0, 0}};
	EXPECT_EQ(1, ring[0].x);
	EXPECT_EQ(-1, ring[1].x);
}

TEST(MathTest, testSmoothstep) {
	EXPECT_NEAR(0.0f, math::smoothstep(0.0f, 1.0f, 0.0f), 0.0001f);
	EXPECT_NEAR(0.5f, math::smoothstep(0.0f, 1.0f, 0.5f), 0.0001f);
	EXPECT_NEAR(1.0f, math::smoothstep(0.0f, 1.0f, 1.0f), 0.0001f);
}

TEST(MathTest, testExp2) {
	EXPECT_NEAR(1.0f, math::exp2(0.0f), 0.0001f);
	EXPECT_NEAR(8.0f, math::exp2(3.0f), 0.0001f);
}

TEST(MathTest, testLdexp) {
	EXPECT_NEAR(8.0f, math::ldexp(1.0f, 3), 0.0001f);
	EXPECT_NEAR(6.0, math::ldexp(1.5, 2), 0.0001);
}

TEST(MathTest, testConstants) {
	EXPECT_TRUE(math::isnan(math::NotANumber));
	EXPECT_TRUE(math::isinf(math::Infinity));
	EXPECT_EQ(2147483647, math::MaxInteger);
	EXPECT_GT(math::MaxFloat, 1.0e30f);
}

TEST(MathTest, testFindMSB) {
	EXPECT_EQ(-1, math::findMSB(0));
	EXPECT_EQ(0, math::findMSB(1));
	EXPECT_EQ(5, math::findMSB(32));
	EXPECT_EQ(8, math::findMSB(256));
	EXPECT_EQ(8, math::findMSB(300));
}

TEST(MathTest, testLogBase2) {
	EXPECT_EQ(0, math::logBase2(1));
	EXPECT_EQ(1, math::logBase2(2));
	EXPECT_EQ(2, math::logBase2(4));
	EXPECT_EQ(3, math::logBase2(8));
	EXPECT_EQ(4, math::logBase2(16));
	EXPECT_EQ(5, math::logBase2(32));
	EXPECT_EQ(10, math::logBase2(1024));
}

TEST(MathTest, testLogBase) {
	EXPECT_EQ(0, math::logBase(10, 1));
	EXPECT_EQ(1, math::logBase(10, 10));
	EXPECT_EQ(2, math::logBase(10, 100));
	EXPECT_EQ(3, math::logBase(10, 1000));
}
