/**
 * @file
 */

#include "scenegraph/FrameTransform.h"
#include "app/tests/AbstractTest.h"
#include "math/tests/TestMathHelper.h"
#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec3f.h"
#include "math/QuatOps.h"
#include "math/Scalar.h"
#include "math/Print.h"

namespace scenegraph {

class FrameTransformTest : public app::AbstractTest {};

TEST_F(FrameTransformTest, testIsIdentity) {
	FrameTransform transform;
	EXPECT_TRUE(transform.isIdentity()) << "Default transform should be identity";
}

TEST_F(FrameTransformTest, testIsIdentityWithTranslation) {
	FrameTransform transform;
	math::Mat4f matrix = math::translate(math::Mat4f(1.0f), math::Vec3f(1.0f, 2.0f, 3.0f));
	transform.setWorldMatrix(matrix);
	EXPECT_FALSE(transform.isIdentity()) << "Transform with translation should not be identity";
}

TEST_F(FrameTransformTest, testIsIdentityWithRotation) {
	FrameTransform transform;
	math::Mat4f matrix = math::rotate(math::Mat4f(1.0f), math::radians(45.0f), math::Vec3f(0.0f, 1.0f, 0.0f));
	transform.setWorldMatrix(matrix);
	EXPECT_FALSE(transform.isIdentity()) << "Transform with rotation should not be identity";
}

TEST_F(FrameTransformTest, testWorldTranslation) {
	FrameTransform transform;
	math::Vec3f translation(7.0f, 8.0f, 9.0f);
	math::Mat4f matrix = math::translate(math::Mat4f(1.0f), translation);
	transform.setWorldMatrix(matrix);

	math::Vec3f result = transform.worldTranslation();
	EXPECT_TRUE(math::all(math::epsilonEqual(translation, result, 0.001f)))
		<< "World translation should be " << math::to_string(translation) << " but got " << math::to_string(result);
}

TEST_F(FrameTransformTest, testWorldScale) {
	FrameTransform transform;
	math::Vec3f scale(2.0f, 3.0f, 4.0f);
	math::Mat4f matrix = math::scale(math::Mat4f(1.0f), scale);
	transform.setWorldMatrix(matrix);

	const math::Vec3f &result = transform.worldScale();
	EXPECT_TRUE(math::all(math::epsilonEqual(scale, result, 0.001f)))
		<< "World scale should be " << math::to_string(scale) << " but got " << math::to_string(result);
}

TEST_F(FrameTransformTest, testWorldScaleCacheInvalidation) {
	FrameTransform transform;
	math::Vec3f scale1(2.0f, 3.0f, 4.0f);
	math::Mat4f matrix1 = math::scale(math::Mat4f(1.0f), scale1);
	transform.setWorldMatrix(matrix1);

	const math::Vec3f &result1 = transform.worldScale();
	EXPECT_TRUE(math::all(math::epsilonEqual(scale1, result1, 0.001f)));

	// Change matrix - cache should be invalidated
	math::Vec3f scale2(5.0f, 6.0f, 7.0f);
	math::Mat4f matrix2 = math::scale(math::Mat4f(1.0f), scale2);
	transform.setWorldMatrix(matrix2);

	const math::Vec3f &result2 = transform.worldScale();
	EXPECT_TRUE(math::all(math::epsilonEqual(scale2, result2, 0.001f)))
		<< "Scale should be updated after setting new matrix";
}

TEST_F(FrameTransformTest, testDecompose) {
	FrameTransform transform;

	math::Vec3f expectedScale(2.0f, 3.0f, 4.0f);
	math::Vec3f expectedTranslation(10.0f, 20.0f, 30.0f);
	math::Quatf expectedOrientation = math::angleAxis(math::radians(45.0f), math::Vec3f(0.0f, 1.0f, 0.0f));

	math::Mat4f matrix = math::translate(math::Mat4f(1.0f), expectedTranslation) *
					   math::mat4_cast(expectedOrientation) *
					   math::scale(math::Mat4f(1.0f), expectedScale);
	transform.setWorldMatrix(matrix);

	math::Vec3f scale;
	math::Quatf orientation;
	math::Vec3f translation;
	transform.decompose(scale, orientation, translation);

	EXPECT_TRUE(math::all(math::epsilonEqual(expectedScale, scale, 0.001f)))
		<< "Decomposed scale should match";
	EXPECT_TRUE(math::all(math::epsilonEqual(expectedTranslation, translation, 0.001f)))
		<< "Decomposed translation should match";
	EXPECT_TRUE(math::all(math::epsilonEqual(orientation, expectedOrientation, 0.001f)))
		<< "Decomposed orientation should match";
}

TEST_F(FrameTransformTest, testCalculateWorldMatrixWithPivot) {
	FrameTransform transform;
	math::Vec3f translation(10.0f, 20.0f, 30.0f);
	math::Mat4f matrix = math::translate(math::Mat4f(1.0f), translation);
	transform.setWorldMatrix(matrix);

	math::Vec3f normalizedPivot(0.5f, 0.5f, 0.5f);
	math::Vec3f dimensions(10.0f, 20.0f, 30.0f);

	math::Mat4f result = transform.calculateWorldMatrix(normalizedPivot, dimensions);

	// The result should have the pivot offset applied
	math::Vec3f pivotOffset = normalizedPivot * dimensions;
	math::Vec3f expectedTranslation = translation - pivotOffset;
	math::Vec3f resultTranslation(result[3]);

	EXPECT_TRUE(math::all(math::epsilonEqual(expectedTranslation, resultTranslation, 0.001f)))
		<< "Calculate world matrix should apply pivot offset";
}

TEST_F(FrameTransformTest, testCalculateWorldMatrixWithZeroPivot) {
	FrameTransform transform;
	math::Vec3f translation(10.0f, 20.0f, 30.0f);
	math::Mat4f matrix = math::translate(math::Mat4f(1.0f), translation);
	transform.setWorldMatrix(matrix);

	math::Vec3f normalizedPivot(0.0f, 0.0f, 0.0f);
	math::Vec3f dimensions(10.0f, 20.0f, 30.0f);

	math::Mat4f result = transform.calculateWorldMatrix(normalizedPivot, dimensions);

	// With zero pivot, result should equal original matrix
	for (int i = 0; i < 4; ++i) {
		EXPECT_TRUE(math::all(math::epsilonEqual(matrix[i], result[i], 0.001f)))
			<< "Calculate world matrix with zero pivot column " << i << " should equal original";
	}
}

TEST_F(FrameTransformTest, testCalculateWorldMatrixWithRotation) {
	FrameTransform transform;
	math::Vec3f translation(10.0f, 20.0f, 30.0f);
	math::Mat4f rotation = math::rotate(math::Mat4f(1.0f), math::radians(90.0f), math::Vec3f(0.0f, 1.0f, 0.0f));
	math::Mat4f matrix = math::translate(math::Mat4f(1.0f), translation) * rotation;
	transform.setWorldMatrix(matrix);

	math::Vec3f normalizedPivot(0.5f, 0.5f, 0.5f);
	math::Vec3f dimensions(10.0f, 20.0f, 30.0f);

	math::Mat4f result = transform.calculateWorldMatrix(normalizedPivot, dimensions);

	// The rotation should be preserved, only pivot offset applied to translation
	math::Vec3f pivotOffset = normalizedPivot * dimensions;
	math::Mat4f expected = matrix * math::translate(math::Mat4f(1.0f), -pivotOffset);

	for (int i = 0; i < 4; ++i) {
		EXPECT_TRUE(math::all(math::epsilonEqual(expected[i], result[i], 0.001f)))
			<< "Calculate world matrix column " << i << " should preserve rotation and apply pivot offset";
	}
}

TEST_F(FrameTransformTest, testComplexTransform) {
	FrameTransform transform;

	// Create a complex transform: translate, rotate, scale
	math::Vec3f translation(5.0f, 10.0f, 15.0f);
	math::Vec3f scale(2.0f, 2.0f, 2.0f);
	math::Quatf rotation = math::angleAxis(math::radians(45.0f), math::normalize(math::Vec3f(1.0f, 1.0f, 0.0f)));

	math::Mat4f matrix = math::translate(math::Mat4f(1.0f), translation) *
					   math::mat4_cast(rotation) *
					   math::scale(math::Mat4f(1.0f), scale);
	transform.setWorldMatrix(matrix);

	EXPECT_FALSE(transform.isIdentity());

	math::Vec3f resultTranslation = transform.worldTranslation();
	EXPECT_TRUE(math::all(math::epsilonEqual(translation, resultTranslation, 0.001f)));

	const math::Vec3f &resultScale = transform.worldScale();
	EXPECT_TRUE(math::all(math::epsilonEqual(scale, resultScale, 0.001f)));

	math::Vec3f decompScale;
	math::Quatf decompRotation;
	math::Vec3f decompTranslation;
	transform.decompose(decompScale, decompRotation, decompTranslation);

	EXPECT_TRUE(math::all(math::epsilonEqual(scale, decompScale, 0.001f)));
	EXPECT_TRUE(math::all(math::epsilonEqual(translation, decompTranslation, 0.001f)));
	EXPECT_TRUE(math::all(math::epsilonEqual(rotation, decompRotation, 0.001f)));
}

} // namespace scenegraph
