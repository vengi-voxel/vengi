/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxel/Face.h"
#include "voxel/Region.h"

namespace voxel {

class FaceTest: public app::AbstractTest {
};

TEST_F(FaceTest, testNegativeX) {
	glm::vec3 rayOrigin { 0 };
	glm::ivec3 hitPos { 14 };
	glm::vec3 rayDirection = glm::normalize(glm::vec3(hitPos) - rayOrigin);
	const FaceNames name = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(FaceNames::NegativeX, name)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) name;
}

TEST_F(FaceTest, testPositiveX) {
	glm::vec3 rayOrigin { 31 };
	glm::ivec3 hitPos { 14 };
	glm::vec3 rayDirection = glm::normalize(glm::vec3(hitPos) - rayOrigin);
	const FaceNames name = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(FaceNames::PositiveX, name)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) name;
}

TEST_F(FaceTest, DISABLED_testNegativeY) {
	glm::vec3 rayOrigin { 12, 0, 14 };
	glm::ivec3 hitPos { 15 };
	glm::vec3 rayDirection = glm::normalize(glm::vec3(hitPos) - rayOrigin);
	const FaceNames name = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(FaceNames::NegativeY, name)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) name;
}

TEST_F(FaceTest, DISABLED_testPositiveY) {
	glm::vec3 rayOrigin { 12, 31, 14 };
	glm::ivec3 hitPos { 15 };
	glm::vec3 rayDirection = glm::normalize(glm::vec3(hitPos) - rayOrigin);
	const FaceNames name = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(FaceNames::PositiveY, name)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) name;
}

TEST_F(FaceTest, DISABLED_testNegativeZ) {
	glm::vec3 rayOrigin { 12, 14, 0 };
	glm::ivec3 hitPos { 15 };
	glm::vec3 rayDirection = glm::normalize(glm::vec3(hitPos) - rayOrigin);
	const FaceNames name = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(FaceNames::NegativeZ, name)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) name;
}

TEST_F(FaceTest, DISABLED_testPositiveZ) {
	glm::vec3 rayOrigin { 12, 14, 31 };
	glm::ivec3 hitPos { 15 };
	glm::vec3 rayDirection = glm::normalize(glm::vec3(hitPos) - rayOrigin);
	const FaceNames name = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(FaceNames::PositiveZ, name)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) name;
}

}
