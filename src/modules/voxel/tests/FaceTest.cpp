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
	glm::vec3 rayOrigin { 0.0f };
	glm::vec3 hitPos { 14.0f };
	const FaceNames name = raycastFaceDetection(rayOrigin, hitPos);
	ASSERT_EQ(FaceNames::NegativeX, name)
		<< "Ray did not hit the expected face. Face: " << (int) name;
}

TEST_F(FaceTest, testPositiveX) {
	glm::vec3 rayOrigin { 31.0f };
	glm::vec3 hitPos { 14.0f };
	glm::vec3 rayDirection = glm::normalize(hitPos - rayOrigin);
	const FaceNames name = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(FaceNames::PositiveX, name)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) name;
}

TEST_F(FaceTest, testNegativeY) {
	glm::vec3 rayOrigin { 12.0f, 0.0f, 14.0f };
	glm::vec3 hitPos { 15.0f };
	glm::vec3 rayDirection = glm::normalize(hitPos - rayOrigin);
	const FaceNames name = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(FaceNames::NegativeY, name)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) name;
}

TEST_F(FaceTest, testPositiveY) {
	glm::vec3 rayOrigin { 12.0f, 31.0f, 14.0f };
	glm::vec3 hitPos { 15.0f };
	glm::vec3 rayDirection = glm::normalize(hitPos - rayOrigin);
	const FaceNames name = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(FaceNames::PositiveY, name)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) name;
}

TEST_F(FaceTest, testNegativeZ) {
	glm::vec3 rayOrigin { 12.0f, 14.0f, 0.0f };
	glm::vec3 hitPos { 15.0f };
	glm::vec3 rayDirection = glm::normalize(hitPos - rayOrigin);
	const FaceNames name = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(FaceNames::NegativeZ, name)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) name;
}

TEST_F(FaceTest, testPositiveZ) {
	glm::vec3 rayOrigin { 12.0f, 14.0f, 31.0f };
	glm::vec3 hitPos { 15.0f };
	glm::vec3 rayDirection = glm::normalize(hitPos - rayOrigin);
	const FaceNames name = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(FaceNames::PositiveZ, name)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) name;
}

}
