/**
 * @file
 */

#include "voxelutil/VolumeRescaler.h"
#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class VolumeRescalerTest : public app::AbstractTest {
protected:
	void testScaleUpFull(int lower, int upper) {
		voxel::RawVolume volume({lower, upper});
		auto func = [&](int x, int y, int z, const voxel::Voxel &voxel) {
			volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		};
		const int n = voxelutil::visitVolumeParallel(volume, func, VisitAll());
		ASSERT_GT(n, 0);
		core::ScopedPtr<voxel::RawVolume> v(voxelutil::scaleUp(volume));
		ASSERT_TRUE(v);
		const voxel::Region scaledRegion = v->region();
		EXPECT_EQ(v->region().voxels(), n * 8);
		const glm::ivec3 dims = scaledRegion.getDimensionsInVoxels();
		const glm::ivec3 mins = scaledRegion.getLowerCorner();
		ASSERT_EQ(dims, volume.region().getDimensionsInVoxels() * 2);
		ASSERT_EQ(mins, volume.region().getLowerCorner());
		EXPECT_EQ(voxelutil::countVoxels(*v), n * 8) << "Expected " << n * 8 << " voxels, but got " << countVoxels(*v);
	}
};

TEST_F(VolumeRescalerTest, testScaleUpEmpty) {
	voxel::RawVolume volume({-8, 8});
	core::ScopedPtr<voxel::RawVolume> v(voxelutil::scaleUp(volume));
	ASSERT_TRUE(v);
	const voxel::Region scaledRegion = v->region();
	const glm::ivec3 dims = scaledRegion.getDimensionsInVoxels();
	const glm::ivec3 mins = scaledRegion.getLowerCorner();
	ASSERT_EQ(dims, volume.region().getDimensionsInVoxels() * 2);
	ASSERT_EQ(mins, volume.region().getLowerCorner());
}

TEST_F(VolumeRescalerTest, testScaleUpFull) {
	testScaleUpFull(-8, 8);
	testScaleUpFull(7, 8);
}

TEST_F(VolumeRescalerTest, testScaleDown) {
	voxel::RawVolume volume({-8, 8});
	for (int y = -8; y <= 8; ++y) {
		for (int x = -2; x <= 2; ++x) {
			for (int z = -2; z <= 2; ++z) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, y + 8));
			}
		}
	}
	palette::Palette pal;
	pal.nippon();
	ASSERT_EQ(5 * 5 * 17, voxelutil::countVoxels(volume));
	const voxel::Region &srcRegion = volume.region();
	const glm::ivec3 &targetDimensionsHalf = (srcRegion.getDimensionsInVoxels() / 2) - 1;
	const voxel::Region destRegion(srcRegion.getLowerCorner(), srcRegion.getLowerCorner() + targetDimensionsHalf);
	voxel::RawVolume destVolume(destRegion);
	voxelutil::scaleDown(volume, pal, destVolume);
	ASSERT_EQ(32, voxelutil::countVoxels(destVolume));
}

TEST_F(VolumeRescalerTest, testScaleVolumeDoubleSize) {
	voxel::RawVolume volume({0, 3});
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	volume.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	volume.setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 3));
	core::ScopedPtr<voxel::RawVolume> scaled(voxelutil::scaleVolume(&volume, glm::vec3(2.0f)));
	ASSERT_TRUE(scaled);
	const voxel::Region &scaledRegion = scaled->region();
	const glm::ivec3 dims = scaledRegion.getDimensionsInVoxels();
	// Source is 4x4x4 (0-3), scaled by 2 should be 8x8x8
	EXPECT_EQ(dims.x, 8);
	EXPECT_EQ(dims.y, 8);
	EXPECT_EQ(dims.z, 8);
	// Check that we have voxels at scaled positions
	EXPECT_FALSE(voxel::isAir(scaled->voxel(0, 0, 0).getMaterial()));
}

TEST_F(VolumeRescalerTest, testScaleVolumeHalfSize) {
	voxel::RawVolume volume({0, 7});
	// Fill a 2x2x2 cube to ensure we get at least one voxel after scaling down
	for (int z = 0; z < 2; ++z) {
		for (int y = 0; y < 2; ++y) {
			for (int x = 0; x < 2; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}
	core::ScopedPtr<voxel::RawVolume> scaled(voxelutil::scaleVolume(&volume, glm::vec3(0.5f)));
	ASSERT_TRUE(scaled);
	const voxel::Region &scaledRegion = scaled->region();
	const glm::ivec3 dims = scaledRegion.getDimensionsInVoxels();
	// Source is 8x8x8 (0-7), scaled by 0.5 should be 4x4x4
	EXPECT_EQ(dims.x, 4);
	EXPECT_EQ(dims.y, 4);
	EXPECT_EQ(dims.z, 4);
	const int voxelCount = voxelutil::countVoxels(*scaled);
	EXPECT_GT(voxelCount, 0);
}

TEST_F(VolumeRescalerTest, testScaleVolumeFractional) {
	voxel::RawVolume volume({0, 3});
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	volume.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	volume.setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 3));
	volume.setVoxel(3, 3, 3, voxel::createVoxel(voxel::VoxelType::Generic, 4));
	core::ScopedPtr<voxel::RawVolume> scaled(voxelutil::scaleVolume(&volume, glm::vec3(1.5f)));
	ASSERT_TRUE(scaled);
	const voxel::Region &scaledRegion = scaled->region();
	const glm::ivec3 dims = scaledRegion.getDimensionsInVoxels();
	EXPECT_EQ(dims.x, 6);
	EXPECT_EQ(dims.y, 6);
	EXPECT_EQ(dims.z, 6);
	const int voxelCount = voxelutil::countVoxels(*scaled);
	EXPECT_GT(voxelCount, 0);
}

TEST_F(VolumeRescalerTest, testScaleVolumeNonUniform) {
	voxel::RawVolume volume({0, 3});
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	volume.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	core::ScopedPtr<voxel::RawVolume> scaled(voxelutil::scaleVolume(&volume, glm::vec3(2.0f, 1.0f, 0.5f)));
	ASSERT_TRUE(scaled);
	const voxel::Region &scaledRegion = scaled->region();
	const glm::ivec3 dims = scaledRegion.getDimensionsInVoxels();
	// Source is 4x4x4 (0-3), scaled by (2, 1, 0.5) should give (8, 4, 2)
	EXPECT_EQ(dims.x, 8);
	EXPECT_EQ(dims.y, 4);
	EXPECT_EQ(dims.z, 2);
}

TEST_F(VolumeRescalerTest, testScaleVolumeEmpty) {
	voxel::RawVolume volume({0, 3});
	core::ScopedPtr<voxel::RawVolume> scaled(voxelutil::scaleVolume(&volume, glm::vec3(2.0f)));
	ASSERT_TRUE(scaled);
	const int voxelCount = voxelutil::countVoxels(*scaled);
	EXPECT_EQ(voxelCount, 0);
}

TEST_F(VolumeRescalerTest, testScaleVolumeNull) {
	core::ScopedPtr<voxel::RawVolume> scaled(voxelutil::scaleVolume(nullptr, glm::vec3(2.0f)));
	EXPECT_FALSE(scaled);
}

TEST_F(VolumeRescalerTest, testScaleVolumeWithCenterPivot) {
	// Volume from 0 to 3 (4x4x4), center pivot at (0.5, 0.5, 0.5)
	voxel::RawVolume volume({0, 3});
	// Place voxel at center
	volume.setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	const glm::vec3 centerPivot(0.5f, 0.5f, 0.5f);
	core::ScopedPtr<voxel::RawVolume> scaled(voxelutil::scaleVolume(&volume, glm::vec3(2.0f), centerPivot));
	ASSERT_TRUE(scaled);
	// With center pivot, the region should expand equally in all directions
	const voxel::Region &scaledRegion = scaled->region();
	const glm::ivec3 dims = scaledRegion.getDimensionsInVoxels();
	// Source is 4x4x4, scaled by 2 with center pivot should give 8x8x8
	EXPECT_EQ(dims.x, 8);
	EXPECT_EQ(dims.y, 8);
	EXPECT_EQ(dims.z, 8);
	// The center should still have voxels
	const int voxelCount = voxelutil::countVoxels(*scaled);
	EXPECT_GT(voxelCount, 0);
}

TEST_F(VolumeRescalerTest, testScaleVolumeWithCornerPivot) {
	// Volume from 0 to 3 (4x4x4), corner pivot at (0, 0, 0)
	// The pivot is at the center of voxel (0,0,0), so scaling expands in all directions from there
	voxel::RawVolume volume({0, 3});
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	const glm::vec3 cornerPivot(0.0f, 0.0f, 0.0f);
	core::ScopedPtr<voxel::RawVolume> scaled(voxelutil::scaleVolume(&volume, glm::vec3(2.0f), cornerPivot));
	ASSERT_TRUE(scaled);
	// The region expands in all directions from the pivot point
	const voxel::Region &scaledRegion = scaled->region();
	const glm::ivec3 dims = scaledRegion.getDimensionsInVoxels();
	EXPECT_EQ(dims.x, 8);
	EXPECT_EQ(dims.y, 8);
	EXPECT_EQ(dims.z, 8);
	// Voxel at origin should still have a voxel (scaled region includes origin)
	EXPECT_FALSE(voxel::isAir(scaled->voxel(0, 0, 0).getMaterial()));
}

TEST_F(VolumeRescalerTest, testScaleVolumePreservesVoxelCount) {
	// When scaling up by integer factor, voxel count should increase predictably
	voxel::RawVolume volume({0, 1});
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	volume.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	const int originalCount = voxelutil::countVoxels(volume);
	core::ScopedPtr<voxel::RawVolume> scaled(voxelutil::scaleVolume(&volume, glm::vec3(2.0f)));
	ASSERT_TRUE(scaled);
	const int scaledCount = voxelutil::countVoxels(*scaled);
	// Each voxel should become approximately 8 voxels (2^3) when scaling by 2
	EXPECT_GE(scaledCount, originalCount);
}

} // namespace voxelutil
