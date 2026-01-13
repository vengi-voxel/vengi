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

} // namespace voxel
