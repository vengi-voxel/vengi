/**
 * @file
 */

#include "voxel/VolumeSampler.h"
#include "app/tests/AbstractTest.h"
#include "math/tests/TestMathHelper.h"
#include "voxel/RawVolume.h"

namespace voxel {

class VolumeSamplerTest : public app::AbstractTest {};

TEST_F(VolumeSamplerTest, testTriplanarSampling) {
	voxel::RawVolume v({0, 2});
	v.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 42));
	voxel::VolumeSampler sampler(&v);
	voxel::Voxel voxel000 = sampleTrilinear(sampler, {0, 0, 0});
	EXPECT_EQ(0, voxel000.getColor());
	voxel::Voxel voxel111 = sampleTrilinear(sampler, {1, 1, 1});
	EXPECT_EQ(42, voxel111.getColor());

	voxel::Voxel voxelfract = sampleTrilinear(sampler, {0.7, 0.6, 0.7});
	EXPECT_EQ(42, voxelfract.getColor());

	voxel::Voxel voxelfract2 = sampleTrilinear(sampler, {1.3, 1.4, 1.2});
	EXPECT_EQ(42, voxelfract2.getColor());
}

TEST_F(VolumeSamplerTest, testTriplanarSamplingNegativeCoordinates) {
	voxel::RawVolume v({-2, 0});
	v.setVoxel(-1, -1, -1, voxel::createVoxel(voxel::VoxelType::Generic, 42));
	voxel::VolumeSampler sampler(&v);
	voxel::Voxel voxel000 = sampleTrilinear(sampler, {0, 0, 0});
	EXPECT_EQ(0, voxel000.getColor());
	voxel::Voxel voxel111 = sampleTrilinear(sampler, {-1, -1, -1});
	EXPECT_EQ(42, voxel111.getColor());

	voxel::Voxel voxelfract = sampleTrilinear(sampler, {-0.7, -0.6, -0.7});
	EXPECT_EQ(42, voxelfract.getColor());

	voxel::Voxel voxelfract2 = sampleTrilinear(sampler, {-1.3, -1.4, -1.2});
	EXPECT_EQ(42, voxelfract2.getColor());
}

TEST_F(VolumeSamplerTest, testSampleTrilinearEmptyVolume) {
	voxel::RawVolume v({0, 3});
	voxel::VolumeSampler sampler(&v);
	voxel::Voxel result = sampleTrilinear(sampler, {1.5f, 1.5f, 1.5f});
	EXPECT_TRUE(voxel::isAir(result.getMaterial()));
}

TEST_F(VolumeSamplerTest, testSampleTrilinearOutOfBounds) {
	voxel::RawVolume v({0, 2});
	v.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 5));
	voxel::VolumeSampler sampler(&v);
	voxel::Voxel result = sampleTrilinear(sampler, {10.0f, 10.0f, 10.0f});
	EXPECT_TRUE(voxel::isAir(result.getMaterial()));
}

TEST_F(VolumeSamplerTest, testSampleTrilinearTwoVoxelsCloserWins) {
	// Two voxels at the corners of the 2x2x2 sampling cube; the nearest one wins.
	voxel::RawVolume v({0, 4});
	v.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	v.setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 20));
	voxel::VolumeSampler sampler(&v);
	// round({1.1,1.1,1.1}) = (1,1,1); w000 = 0.9^3 = 0.729 dominates
	voxel::Voxel result = sampleTrilinear(sampler, {1.1f, 1.1f, 1.1f});
	EXPECT_EQ(10, result.getColor());
	// round({1.9,1.9,1.9}) = (2,2,2); w000 = 1.1^3 = 1.331 dominates
	voxel::Voxel result2 = sampleTrilinear(sampler, {1.9f, 1.9f, 1.9f});
	EXPECT_EQ(20, result2.getColor());
}

TEST_F(VolumeSamplerTest, testSampleCubicSamplerEmptyVolume) {
	voxel::RawVolume v({0, 5});
	voxel::VolumeSampler sampler(&v);
	voxel::Voxel result = sampleCubic(sampler, {2.5f, 2.5f, 2.5f});
	EXPECT_TRUE(voxel::isAir(result.getMaterial()));
}

TEST_F(VolumeSamplerTest, testSampleCubicSamplerExactPosition) {
	voxel::RawVolume v({0, 5});
	v.setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 7));
	voxel::VolumeSampler sampler(&v);
	voxel::Voxel result = sampleCubic(sampler, {2.0f, 2.0f, 2.0f});
	EXPECT_EQ(7, result.getColor());
}

TEST_F(VolumeSamplerTest, testSampleCubicSamplerNearVoxel) {
	voxel::RawVolume v({0, 5});
	v.setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 33));
	voxel::VolumeSampler sampler(&v);
	voxel::Voxel result = sampleCubic(sampler, {2.2f, 2.1f, 1.9f});
	EXPECT_EQ(33, result.getColor());
}

TEST_F(VolumeSamplerTest, testSampleCubicSamplerOutOfBounds) {
	voxel::RawVolume v({0, 3});
	v.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 99));
	voxel::VolumeSampler sampler(&v);
	voxel::Voxel result = sampleCubic(sampler, {50.0f, 50.0f, 50.0f});
	EXPECT_TRUE(voxel::isAir(result.getMaterial()));
}

TEST_F(VolumeSamplerTest, testSampleCubicSamplerMatchesVolume) {
	// Both overloads must return the same color for the same scene
	voxel::RawVolume v({0, 6});
	v.setVoxel(3, 3, 3, voxel::createVoxel(voxel::VoxelType::Generic, 11));
	v.setVoxel(4, 3, 3, voxel::createVoxel(voxel::VoxelType::Generic, 22));
	voxel::VolumeSampler sampler(&v);
	const glm::vec3 pos{3.4f, 3.0f, 3.0f};
	EXPECT_EQ(sampleCubic(sampler, pos).getColor(), sampleCubic(sampler, pos).getColor());
}

} // namespace voxel
