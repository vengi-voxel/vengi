/**
 * @file
 */

#include "voxelutil/Raycast.h"
#include "app/tests/AbstractTest.h"
#include "core/GLMConst.h"
#include "core/collection/DynamicArray.h"
#include "voxel/RawVolume.h"
#include "gtest/gtest.h"

namespace voxelutil {

class RaycastTest : public app::AbstractTest {
protected:
	/**
	 * @brief Simple callback that counts visited voxels and stops on solid voxels
	 */
	class SimpleRaycastFunctor {
	public:
		int visitedVoxels = 0;
		glm::ivec3 lastPosition{0};
		bool hitSolid = false;
		glm::ivec3 hitPosition{0};

		template<typename Sampler>
		bool operator()(Sampler &sampler) {
			visitedVoxels++;
			lastPosition = sampler.position();

			if (!voxel::isAir(sampler.voxel().getMaterial())) {
				hitSolid = true;
				hitPosition = sampler.position();
				return false; // Stop raycast
			}
			return true; // Continue raycast
		}
	};

	/**
	 * @brief Callback that just counts all visited voxels
	 */
	class CountingRaycastFunctor {
	public:
		int visitedVoxels = 0;
		core::DynamicArray<glm::ivec3> visitedPositions;

		template<typename Sampler>
		bool operator()(Sampler &sampler) {
			visitedVoxels++;
			visitedPositions.push_back(sampler.position());
			return true; // Always continue
		}
	};

	/**
	 * @brief Creates a test volume with some predefined solid voxels
	 */
	voxel::RawVolume createTestVolume(const voxel::Region &region) {
		voxel::RawVolume volume(region);

		// Place solid voxels accounting for raycast algorithm behavior
		// These positions correspond to where rays at integer+0.5 coordinates will actually visit
		volume.setVoxel(glm::ivec3(5, 6, 6), voxel::createVoxel(voxel::VoxelType::Generic, 1)); // For X,Y rays at 5.5
		volume.setVoxel(glm::ivec3(3, 4, 4), voxel::createVoxel(voxel::VoxelType::Generic, 2)); // For X rays at 3.5
		volume.setVoxel(glm::ivec3(7, 3, 5), voxel::createVoxel(voxel::VoxelType::Generic, 3)); // For Z rays at 2.5
		volume.setVoxel(glm::ivec3(4, 4, 4), voxel::createVoxel(voxel::VoxelType::Generic, 4)); // For diagonal rays

		return volume;
	}
};

TEST_F(RaycastTest, testRaycastWithEndpointsEmptyVolume) {
	// Test raycast through an empty volume
	voxel::RawVolume volume({0, 10});

	const glm::vec3 start(0.5f, 0.5f, 0.5f);
	const glm::vec3 end(10.5f, 10.5f, 10.5f);

	CountingRaycastFunctor functor;
	RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

	EXPECT_TRUE(result.isCompleted()) << "Should complete without hitting anything in an empty volume";
	EXPECT_GT(functor.visitedVoxels, 0) << "Should visit at least some voxels";
	EXPECT_LT(functor.visitedVoxels, 50) << "Should not visit excessive voxels";
}

TEST_F(RaycastTest, testNegativeXStraightOffsets) {
	glm::vec3 rayOrigin { 0.0f, 0.0f, 0.0f };
	glm::vec3 hitPos { 14.0f, 0.0f, 0.0f };
	const RaycastHit hit = raycastFaceDetection(rayOrigin, hitPos, 0.0f, 1.0f);
	ASSERT_EQ(voxel::FaceNames::NegativeX, hit.face)
		<< "Ray did not hit the expected face. Face: " << (int) hit.face;
}

TEST_F(RaycastTest, testPositiveXStraight) {
	glm::vec3 rayOrigin { 0.0f, 0.0f, 0.0f };
	glm::vec3 hitPos { -14.0f, 0.0f, 0.0f };
	const RaycastHit hit = raycastFaceDetection(rayOrigin, hitPos);
	ASSERT_EQ(voxel::FaceNames::PositiveX, hit.face)
		<< "Ray did not hit the expected face. Face: " << (int) hit.face;
}

TEST_F(RaycastTest, testNegativeXStraight) {
	glm::vec3 rayOrigin { 0.0f, 0.0f, 0.0f };
	glm::vec3 hitPos { 14.0f, 0.0f, 0.0f };
	const RaycastHit hit = raycastFaceDetection(rayOrigin, hitPos);
	ASSERT_EQ(voxel::FaceNames::NegativeX, hit.face)
		<< "Ray did not hit the expected face. Face: " << (int) hit.face;
}

TEST_F(RaycastTest, testNegativeX) {
	glm::vec3 rayOrigin { 0.0f };
	glm::vec3 hitPos { 14.0f };
	const RaycastHit hit = raycastFaceDetection(rayOrigin, hitPos);
	ASSERT_EQ(voxel::FaceNames::NegativeX, hit.face)
		<< "Ray did not hit the expected face. Face: " << (int) hit.face;
}

TEST_F(RaycastTest, testPositiveX) {
	glm::vec3 rayOrigin { 31.0f };
	glm::vec3 hitPos { 14.0f };
	glm::vec3 rayDirection = glm::normalize(hitPos - rayOrigin);
	const RaycastHit hit = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(voxel::FaceNames::PositiveX, hit.face)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) hit.face;
}

TEST_F(RaycastTest, testNegativeY) {
	glm::vec3 rayOrigin { 12.0f, 0.0f, 14.0f };
	glm::vec3 hitPos { 15.0f };
	glm::vec3 rayDirection = glm::normalize(hitPos - rayOrigin);
	const RaycastHit hit = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(voxel::FaceNames::NegativeY, hit.face)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) hit.face;
}

TEST_F(RaycastTest, testPositiveY) {
	glm::vec3 rayOrigin { 12.0f, 31.0f, 14.0f };
	glm::vec3 hitPos { 15.0f };
	glm::vec3 rayDirection = glm::normalize(hitPos - rayOrigin);
	const RaycastHit hit = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(voxel::FaceNames::PositiveY, hit.face)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) hit.face;
}

TEST_F(RaycastTest, testNegativeZ) {
	glm::vec3 rayOrigin { 12.0f, 14.0f, 0.0f };
	glm::vec3 hitPos { 15.0f };
	glm::vec3 rayDirection = glm::normalize(hitPos - rayOrigin);
	const RaycastHit hit = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(voxel::FaceNames::NegativeZ, hit.face)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) hit.face;
}

TEST_F(RaycastTest, testPositiveZ) {
	glm::vec3 rayOrigin { 12.0f, 14.0f, 31.0f };
	glm::vec3 hitPos { 15.0f };
	glm::vec3 rayDirection = glm::normalize(hitPos - rayOrigin);
	const RaycastHit hit = raycastFaceDetection(rayOrigin, rayDirection, hitPos);
	ASSERT_EQ(voxel::FaceNames::PositiveZ, hit.face)
		<< "Ray did not hit the expected face. Direction is "
		<< rayDirection.x << ":" << rayDirection.y << ":" << rayDirection.z
		<< ", Face: " << (int) hit.face;
}

TEST_F(RaycastTest, testRaycastWithEndpointsHitSolidVoxel) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	const glm::vec3 start(1.5f, 6.5f, 6.5f);
	const glm::vec3 end(10.5f, 5.5f, 5.5f); // Ray passes through (5, 6, 6)

	SimpleRaycastFunctor functor;
	RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

	EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_GT(functor.visitedVoxels, 0) << "Should visit at least some voxels";
}

TEST_F(RaycastTest, testRaycastWithEndpointsSameStartEnd) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	const glm::vec3 start(2.5f, 2.5f, 2.5f);
	const glm::vec3 end(2.5f, 2.5f, 2.5f);

	CountingRaycastFunctor functor;
	RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

	EXPECT_TRUE(result.isCompleted()) << "Should complete without hitting anything since start == end";
	EXPECT_EQ(1, functor.visitedVoxels) << "Should visit exactly one voxel when start == end";
}

TEST_F(RaycastTest, testRaycastWithEndpointsAxisAligned) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	// X-axis raycast
	{
		const glm::vec3 start(1.5f, 4.5f, 4.5f);
		const glm::vec3 end(11.5f, 4.5f, 4.5f);

		SimpleRaycastFunctor functor;
		RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

		EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (4, 4, 4)";
		EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (4, 4, 4)";
		EXPECT_EQ(glm::ivec3(3, 4, 4), functor.hitPosition) << "Should hit the solid voxel at (4, 4, 4)";
	}

	// Y-axis raycast
	{
		const glm::vec3 start(5.5f, 1.5f, 6.5f);
		const glm::vec3 end(5.5f, 11.5f, 6.5f);

		SimpleRaycastFunctor functor;
		RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

		EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (5, 6, 6)";
		EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (5, 6, 6)";
		EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition) << "Should hit the solid voxel at (5, 6, 6)";
	}

	// Z-axis raycast
	{
		const glm::vec3 start(7.5f, 3.5f, 1.5f);
		const glm::vec3 end(7.5f, 3.5f, 11.5f);

		SimpleRaycastFunctor functor;
		RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

		EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (7, 3, 5)";
		EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (7, 3, 5)";
		EXPECT_EQ(glm::ivec3(7, 3, 5), functor.hitPosition) << "Should hit the solid voxel at (7, 3, 5)";
	}
}

TEST_F(RaycastTest, testRaycastWithEndpointsNegativeDirection) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	const glm::vec3 start(16.5f, 6.5f, 6.5f); // Start further out to reach the voxel
	const glm::vec3 end(-6.5f, 6.5f, 6.5f);	  // Ray passes through (5, 6, 6) from positive direction

	SimpleRaycastFunctor functor;
	RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

	EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition) << "Should hit the solid voxel at (5, 6, 6)";
}

TEST_F(RaycastTest, testRaycastWithEndpointsDiagonalRay) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	const glm::vec3 start(1.5f, 1.5f, 1.5f);
	const glm::vec3 end(6.5f, 6.5f, 6.5f); // Should pass through some solid voxels

	SimpleRaycastFunctor functor;
	RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

	EXPECT_TRUE(result.isInterrupted()) << "Should hit the first solid voxel in the diagonal path - (4, 4, 4)";
	EXPECT_TRUE(functor.hitSolid) << "Should hit the first solid voxel in the diagonal path - (4, 4, 4)";
	EXPECT_EQ(glm::ivec3(4, 4, 4), functor.hitPosition)
		<< "Should hit the first solid voxel in the diagonal path - (4, 4, 4)";
}

TEST_F(RaycastTest, testRaycastWithDirectionBasicFunctionality) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	const glm::vec3 start(1.5f, 6.5f, 6.5f);
	const glm::vec3 direction(10.0f, 0.0f, 0.0f); // 10 units in positive X direction

	SimpleRaycastFunctor functor;
	RaycastResult result = raycastWithDirection(&volume, start, direction, functor);

	EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition) << "Should hit the solid voxel at (5, 6, 6)";
}

// Test raycast with direction that doesn't reach any solid voxels
TEST_F(RaycastTest, testRaycastWithDirectionShortRay) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	const glm::vec3 start(0.5f, 5.5f, 5.5f);
	const glm::vec3 direction(2.0f, 0.0f, 0.0f); // Only 2 units, won't reach (5, 6, 6)

	CountingRaycastFunctor functor;
	RaycastResult result = raycastWithDirection(&volume, start, direction, functor);

	EXPECT_TRUE(result.isCompleted()) << "Should complete without hitting anything since ray is too short";
	EXPECT_GT(functor.visitedVoxels, 0) << "Should visit at least some voxels";
	EXPECT_LE(functor.visitedVoxels, 5) << "Should visit at most a few voxels";
}

// Test with normalized direction vector (should only visit one voxel)
TEST_F(RaycastTest, testRaycastWithDirectionNormalizedDirection) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	const glm::vec3 start(5.5f, 6.5f, 6.5f); // Start at solid voxel position
	const glm::vec3 direction(1.0f, 0.0f, 0.0f);

	SimpleRaycastFunctor functor;
	RaycastResult result = raycastWithDirection(&volume, start, direction, functor);

	EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel immediately";
	EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel immediately";
	EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_EQ(1, functor.visitedVoxels) << "Normalized direction should only visit one voxel";
}

TEST_F(RaycastTest, testRaycastWithEndpointsVolumeSpecializedFunction) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	const glm::vec3 start(1.5f, 4.5f, 4.5f);
	const glm::vec3 end(11.5f, 4.5f, 4.5f);

	SimpleRaycastFunctor functor;
	RaycastResult result = raycastWithEndpointsVolume(&volume, start, end, functor);

	EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (4, 4, 4)";
	EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (4, 4, 4)";
	EXPECT_EQ(glm::ivec3(3, 4, 4), functor.hitPosition) << "Should hit the solid voxel at (4, 4, 4)";
}

TEST_F(RaycastTest, testRaycastConsistencyEndpointsVsDirection) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	const glm::vec3 start(0.5f, 5.5f, 5.5f);
	const glm::vec3 end(10.5f, 5.5f, 5.5f);
	const glm::vec3 direction = end - start;

	SimpleRaycastFunctor functor1;
	RaycastResult result1 = raycastWithEndpoints(&volume, start, end, functor1);

	SimpleRaycastFunctor functor2;
	RaycastResult result2 = raycastWithDirection(&volume, start, direction, functor2);

	EXPECT_EQ(result1.type, result2.type) << "Raycast results should be the same for both methods";
	EXPECT_EQ(functor1.hitSolid, functor2.hitSolid) << "Hit status should be the same for both methods";
	EXPECT_EQ(functor1.hitPosition, functor2.hitPosition) << "Hit position should be the same for both methods";
	EXPECT_EQ(functor1.visitedVoxels, functor2.visitedVoxels)
		<< "Visited voxel count should be the same for both methods";
}

TEST_F(RaycastTest, testRaycastEdgeCases) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	// Test ray starting from solid voxel
	{
		const glm::vec3 start(5.5f, 6.5f, 6.5f);
		const glm::vec3 end(8.5f, 6.5f, 6.5f);

		SimpleRaycastFunctor functor;
		RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

		EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (5, 6, 6)";
		EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (5, 6, 6)";
		EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition) << "Should hit the solid voxel at (5, 6, 6)";
	}

	// Test ray ending at solid voxel
	{
		const glm::vec3 start(3.5f, 6.5f, 6.5f);
		const glm::vec3 end(6.5f, 6.5f, 6.5f);

		SimpleRaycastFunctor functor;
		RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

		EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (5, 6, 6)";
		EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (5, 6, 6)";
		EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition) << "Should hit the solid voxel at (5, 6, 6)";
	}
}

// Test that raycast visits expected voxels in order
TEST_F(RaycastTest, testRaycastVoxelTraversal) {
	voxel::RawVolume volume({0, 10});

	const glm::vec3 start(1.5f, 1.5f, 1.5f);
	const glm::vec3 end(4.5f, 1.5f, 1.5f);

	CountingRaycastFunctor functor;
	RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

	EXPECT_TRUE(result.isCompleted()) << "Should complete without hitting anything in an empty volume";
	EXPECT_GT(functor.visitedVoxels, 0) << "Should visit at least some voxels";

	// Check that visited positions are reasonable
	for (const auto &pos : functor.visitedPositions) {
		EXPECT_GE(pos.x, 1) << "Should start at voxel x=1";
		EXPECT_LE(pos.x, 5) << "Should end at voxel x=5";
		EXPECT_EQ(pos.y, 1) << "Should stay on same Y";
		EXPECT_EQ(pos.z, 1) << "Should stay on same Z";
	}
}

TEST_F(RaycastTest, testRaycastOutOfBounds) {
	voxel::RawVolume volume(voxel::Region(glm::ivec3(0), glm::ivec3(5)));

	const glm::vec3 start(2.5f, 2.5f, 2.5f);
	const glm::vec3 end(10.5f, 10.5f, 10.5f); // Goes outside 5x5x5 volume

	CountingRaycastFunctor functor;
	RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

	EXPECT_TRUE(result.isCompleted()) << "Should complete without hitting anything since volume is empty";
	EXPECT_GT(functor.visitedVoxels, 0) << "Should visit at least some voxels";
}

TEST_F(RaycastTest, testRaycastLengthToSolidVoxelFace) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	const glm::vec3 start(4.5, 6, 6);
	const glm::vec3 end(8, 6, 6);

	SimpleRaycastFunctor functor;
	RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

	EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_FLOAT_EQ(result.length, 0.5f) << "RaycastResult.length should be the distance to the voxel face";
}

// Starting inside a solid voxel should report fract == 0.0
TEST_F(RaycastTest, testRaycastFractStartInsideSolid) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	const glm::vec3 start(5.5f, 6.5f, 6.5f); // inside solid voxel (5,6,6)
	const glm::vec3 end(8.5f, 6.5f, 6.5f);

	SimpleRaycastFunctor functor;
	RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

	EXPECT_TRUE(result.isInterrupted()) << "Should be interrupted immediately when starting inside solid";
	EXPECT_TRUE(result.isSolidStart()) << "fract should indicate solid start (0.0)";
	EXPECT_FLOAT_EQ(result.fract, 0.0f);
	EXPECT_TRUE(functor.hitSolid) << "Functor should report a hit when starting in solid";
	EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition) << "Should hit the solid voxel at (5,6,6)";
}

// When ray hits nothing, fract should be 1.0 and result completed
TEST_F(RaycastTest, testRaycastFractCompletedIsOne) {
	voxel::RawVolume volume({0, 10});

	const glm::vec3 start(0.5f, 0.5f, 0.5f);
	const glm::vec3 end(10.5f, 10.5f, 10.5f);

	CountingRaycastFunctor functor;
	RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

	EXPECT_TRUE(result.isCompleted()) << "Should complete without hitting anything in an empty volume";
	EXPECT_FLOAT_EQ(result.fract, 1.0f);
}

TEST_F(RaycastTest, testRaycastFaceNormalsAndAdjustPoint) {
	voxel::RawVolume volume = createTestVolume({0, 10});

	// Approach voxel (5,6,6) from negative X to positive X (di = 1)
	{
		const glm::vec3 start(1.5f, 6.5f, 6.5f);
		const glm::vec3 end(11.5f, 6.5f, 6.5f);

		SimpleRaycastFunctor functor;
		RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

		EXPECT_TRUE(result.isInterrupted());
		EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition);
		// stepping along +X sets lastNormal = (-1,0,0)
		EXPECT_EQ(glm::ivec3(-1, 0, 0), result.normal) << "Normal should point -X when entering from -X side";

		// collision point reported by raycast is sampler.position() + normal
		const glm::vec3 collisionPoint = glm::vec3(functor.hitPosition) + glm::vec3(result.normal);
		const glm::vec3 adjusted = result.adjustPoint(collisionPoint, 0.5f);
		// For normal (-1,0,0) adjustPoint should move the collision point in +X direction
		EXPECT_GT(adjusted.x, collisionPoint.x);
	}

	// Approach voxel (5,6,6) from positive X to negative X (di = -1)
	{
		const glm::vec3 start(16.5f, 6.5f, 6.5f);
		const glm::vec3 end(-6.5f, 6.5f, 6.5f);

		SimpleRaycastFunctor functor;
		RaycastResult result = raycastWithEndpoints(&volume, start, end, functor);

		EXPECT_TRUE(result.isInterrupted());
		EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition);
		// stepping along -X sets lastNormal = (1,0,0)
		EXPECT_EQ(glm::ivec3(1, 0, 0), result.normal) << "Normal should point +X when entering from +X side";

		const glm::vec3 collisionPoint = glm::vec3(functor.hitPosition) + glm::vec3(result.normal);
		const glm::vec3 adjusted = result.adjustPoint(collisionPoint, 0.5f);
		// For normal (1,0,0) adjustPoint should move the collision point in -X direction
		EXPECT_LT(adjusted.x, collisionPoint.x);
	}
}

TEST_F(RaycastTest, testRaycastFractMiddleHitIsHalf) {
	voxel::RawVolume volume = createTestVolume({0, 10});
	const glm::vec3 start(7.0f, 6.0f, 6.0f);
	const glm::vec3 dir = glm::left() * 2.0f;

	SimpleRaycastFunctor functor;
	RaycastResult result = raycastWithDirection(&volume, start, dir, functor);

	EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition) << "Should hit voxel (5,6,6)";
	EXPECT_FLOAT_EQ(result.fract, 0.5f) << "fract should be approximately 0.5 for this mid-ray hit";
}

TEST_F(RaycastTest, testRaycastFractMiddleHitIsOneThird) {
	voxel::RawVolume volume = createTestVolume({0, 10});
	const glm::vec3 start(7.0f, 6.0f, 6.0f);
	const glm::vec3 dir = glm::left() * 3.0f;

	SimpleRaycastFunctor functor;
	RaycastResult result = raycastWithDirection(&volume, start, dir, functor);

	EXPECT_TRUE(result.isInterrupted()) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_TRUE(functor.hitSolid) << "Should hit the solid voxel at (5, 6, 6)";
	EXPECT_EQ(glm::ivec3(5, 6, 6), functor.hitPosition) << "Should hit voxel (5,6,6)";
	EXPECT_NEAR(result.fract, 0.3333f, 0.0001f) << "fract should be approximately 0.5 for this mid-ray hit";
}

} // namespace voxelutil
