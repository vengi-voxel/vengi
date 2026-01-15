/**
 * @file
 */

#include "scenegraph/FrameTransform.h"
#include "app/tests/AbstractTest.h"
#include "math/tests/TestMathHelper.h"
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>

namespace scenegraph {

class FrameTransformTest : public app::AbstractTest {};

TEST_F(FrameTransformTest, testIsIdentity) {
	FrameTransform transform;
	EXPECT_TRUE(transform.isIdentity()) << "Default transform should be identity";
}

TEST_F(FrameTransformTest, testIsIdentityWithTranslation) {
	FrameTransform transform;
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
	transform.setWorldMatrix(matrix);
	EXPECT_FALSE(transform.isIdentity()) << "Transform with translation should not be identity";
}

TEST_F(FrameTransformTest, testIsIdentityWithRotation) {
	FrameTransform transform;
	glm::mat4 matrix = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	transform.setWorldMatrix(matrix);
	EXPECT_FALSE(transform.isIdentity()) << "Transform with rotation should not be identity";
}

TEST_F(FrameTransformTest, testWorldTranslation) {
	FrameTransform transform;
	glm::vec3 translation(7.0f, 8.0f, 9.0f);
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), translation);
	transform.setWorldMatrix(matrix);

	glm::vec3 result = transform.worldTranslation();
	EXPECT_TRUE(glm::all(glm::epsilonEqual(translation, result, 0.001f)))
		<< "World translation should be " << glm::to_string(translation) << " but got " << glm::to_string(result);
}

TEST_F(FrameTransformTest, testWorldScale) {
	FrameTransform transform;
	glm::vec3 scale(2.0f, 3.0f, 4.0f);
	glm::mat4 matrix = glm::scale(glm::mat4(1.0f), scale);
	transform.setWorldMatrix(matrix);

	const glm::vec3 &result = transform.worldScale();
	EXPECT_TRUE(glm::all(glm::epsilonEqual(scale, result, 0.001f)))
		<< "World scale should be " << glm::to_string(scale) << " but got " << glm::to_string(result);
}

TEST_F(FrameTransformTest, testWorldScaleCacheInvalidation) {
	FrameTransform transform;
	glm::vec3 scale1(2.0f, 3.0f, 4.0f);
	glm::mat4 matrix1 = glm::scale(glm::mat4(1.0f), scale1);
	transform.setWorldMatrix(matrix1);

	const glm::vec3 &result1 = transform.worldScale();
	EXPECT_TRUE(glm::all(glm::epsilonEqual(scale1, result1, 0.001f)));

	// Change matrix - cache should be invalidated
	glm::vec3 scale2(5.0f, 6.0f, 7.0f);
	glm::mat4 matrix2 = glm::scale(glm::mat4(1.0f), scale2);
	transform.setWorldMatrix(matrix2);

	const glm::vec3 &result2 = transform.worldScale();
	EXPECT_TRUE(glm::all(glm::epsilonEqual(scale2, result2, 0.001f)))
		<< "Scale should be updated after setting new matrix";
}

TEST_F(FrameTransformTest, testDecompose) {
	FrameTransform transform;

	glm::vec3 expectedScale(2.0f, 3.0f, 4.0f);
	glm::vec3 expectedTranslation(10.0f, 20.0f, 30.0f);
	glm::quat expectedOrientation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), expectedTranslation) *
					   glm::mat4_cast(expectedOrientation) *
					   glm::scale(glm::mat4(1.0f), expectedScale);
	transform.setWorldMatrix(matrix);

	glm::vec3 scale;
	glm::quat orientation;
	glm::vec3 translation;
	transform.decompose(scale, orientation, translation);

	EXPECT_TRUE(glm::all(glm::epsilonEqual(expectedScale, scale, 0.001f)))
		<< "Decomposed scale should match";
	EXPECT_TRUE(glm::all(glm::epsilonEqual(expectedTranslation, translation, 0.001f)))
		<< "Decomposed translation should match";
	EXPECT_TRUE(glm::all(glm::epsilonEqual(orientation, expectedOrientation, 0.001f)))
		<< "Decomposed orientation should match";
}

TEST_F(FrameTransformTest, testCalculateWorldMatrixWithPivot) {
	FrameTransform transform;
	glm::vec3 translation(10.0f, 20.0f, 30.0f);
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), translation);
	transform.setWorldMatrix(matrix);

	glm::vec3 normalizedPivot(0.5f, 0.5f, 0.5f);
	glm::vec3 dimensions(10.0f, 20.0f, 30.0f);

	glm::mat4 result = transform.calculateWorldMatrix(normalizedPivot, dimensions);

	// The result should have the pivot offset applied
	glm::vec3 pivotOffset = normalizedPivot * dimensions;
	glm::vec3 expectedTranslation = translation - pivotOffset;
	glm::vec3 resultTranslation(result[3]);

	EXPECT_TRUE(glm::all(glm::epsilonEqual(expectedTranslation, resultTranslation, 0.001f)))
		<< "Calculate world matrix should apply pivot offset";
}

TEST_F(FrameTransformTest, testCalculateWorldMatrixWithZeroPivot) {
	FrameTransform transform;
	glm::vec3 translation(10.0f, 20.0f, 30.0f);
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), translation);
	transform.setWorldMatrix(matrix);

	glm::vec3 normalizedPivot(0.0f, 0.0f, 0.0f);
	glm::vec3 dimensions(10.0f, 20.0f, 30.0f);

	glm::mat4 result = transform.calculateWorldMatrix(normalizedPivot, dimensions);

	// With zero pivot, result should equal original matrix
	for (int i = 0; i < 4; ++i) {
		EXPECT_TRUE(glm::all(glm::epsilonEqual(matrix[i], result[i], 0.001f)))
			<< "Calculate world matrix with zero pivot column " << i << " should equal original";
	}
}

TEST_F(FrameTransformTest, testCalculateWorldMatrixWithRotation) {
	FrameTransform transform;
	glm::vec3 translation(10.0f, 20.0f, 30.0f);
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), translation) * rotation;
	transform.setWorldMatrix(matrix);

	glm::vec3 normalizedPivot(0.5f, 0.5f, 0.5f);
	glm::vec3 dimensions(10.0f, 20.0f, 30.0f);

	glm::mat4 result = transform.calculateWorldMatrix(normalizedPivot, dimensions);

	// The rotation should be preserved, only pivot offset applied to translation
	glm::vec3 pivotOffset = normalizedPivot * dimensions;
	glm::mat4 expected = matrix * glm::translate(glm::mat4(1.0f), -pivotOffset);

	for (int i = 0; i < 4; ++i) {
		EXPECT_TRUE(glm::all(glm::epsilonEqual(expected[i], result[i], 0.001f)))
			<< "Calculate world matrix column " << i << " should preserve rotation and apply pivot offset";
	}
}

TEST_F(FrameTransformTest, testComplexTransform) {
	FrameTransform transform;

	// Create a complex transform: translate, rotate, scale
	glm::vec3 translation(5.0f, 10.0f, 15.0f);
	glm::vec3 scale(2.0f, 2.0f, 2.0f);
	glm::quat rotation = glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));

	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), translation) *
					   glm::mat4_cast(rotation) *
					   glm::scale(glm::mat4(1.0f), scale);
	transform.setWorldMatrix(matrix);

	EXPECT_FALSE(transform.isIdentity());

	glm::vec3 resultTranslation = transform.worldTranslation();
	EXPECT_TRUE(glm::all(glm::epsilonEqual(translation, resultTranslation, 0.001f)));

	const glm::vec3 &resultScale = transform.worldScale();
	EXPECT_TRUE(glm::all(glm::epsilonEqual(scale, resultScale, 0.001f)));

	glm::vec3 decompScale;
	glm::quat decompRotation;
	glm::vec3 decompTranslation;
	transform.decompose(decompScale, decompRotation, decompTranslation);

	EXPECT_TRUE(glm::all(glm::epsilonEqual(scale, decompScale, 0.001f)));
	EXPECT_TRUE(glm::all(glm::epsilonEqual(translation, decompTranslation, 0.001f)));
	EXPECT_TRUE(glm::all(glm::epsilonEqual(rotation, decompRotation, 0.001f)));
}

} // namespace scenegraph
