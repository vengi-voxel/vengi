/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "core/Bits.h"
#include "core/Enum.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"

#include <glm/geometric.hpp>

namespace voxel {

inline ::std::ostream& operator<<(::std::ostream& os, const voxel::FaceBits& facebits) {
	return os << "bits[" << core::toBitString(core::enumVal(facebits)) << "]";
}

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

TEST_F(FaceTest, testVisibility) {
	const voxel::Region region(0, 31);
	const voxel::Voxel voxel = createVoxel(voxel::VoxelType::Generic, 1);
	voxel::RawVolume volume(region);
	for (int x = 0; x < 3; ++x) {
		for (int y = 0; y < 3; ++y) {
			for (int z = 0; z < 3; ++z) {
				volume.setVoxel(x, y, z, voxel);
			}
		}
	}


	EXPECT_EQ(FaceBits::NegativeX | FaceBits::NegativeY | FaceBits::NegativeZ, visibleFaces(volume, 0, 0, 0));
	EXPECT_EQ(FaceBits::None, visibleFaces(volume, 1, 1, 1));
	EXPECT_EQ(FaceBits::PositiveX | FaceBits::PositiveY | FaceBits::PositiveZ, visibleFaces(volume, 2, 2, 2));
	EXPECT_EQ(FaceBits::PositiveX | FaceBits::PositiveZ, visibleFaces(volume, 2, 1, 2));
}

}
