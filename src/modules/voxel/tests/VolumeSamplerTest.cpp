/**
 * @file
 */

#include "voxel/VolumeSampler.h"
#include "voxel/VolumeSamplerUtil.h"
#include "app/tests/AbstractTest.h"
#include "math/tests/TestMathHelper.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"

namespace voxel {

class VolumeSamplerTest : public app::AbstractTest {};

TEST_F(VolumeSamplerTest, TrilinearPreservesCornerSelectionAndMetadata) {
	const int previous = RawVolume::chunkedStorageThreshold();
	for (int threshold : {1, 256}) {
		RawVolume::setChunkedStorageThreshold(threshold);
		RawVolume volume(Region(-2, -3, -4, 3, 4, 5));
		RawVolume::setChunkedStorageThreshold(previous);
		volume.setBorderValue(Voxel(VoxelType::Generic, 99));
		for (int z = -4; z <= 5; ++z) {
			for (int y = -3; y <= 4; ++y) {
				for (int x = -2; x <= 3; ++x) {
					volume.setVoxel(x, y, z, Voxel((x + y + z) % 3 == 0 ? VoxelType::Air : VoxelType::Generic,
						(uint8_t)(x + 2 + 6 * (y + 3) + 48 * (z + 4)), 9, FlagOutline, 7));
				}
			}
		}
		const auto compare = [&](auto &sampler, const Region &bounds) {
			for (int z = -12; z <= 16; ++z) {
				for (int y = -12; y <= 16; ++y) {
					for (int x = -12; x <= 16; ++x) {
						const glm::vec3 pos = glm::vec3(x, y, z) * 0.25f;
						const glm::ivec3 base = glm::ivec3(glm::round(pos));
						const Voxel expected = bounds.containsPoint(base) ? volume.voxel(base) : Voxel();
						EXPECT_EQ(expected, sampleTrilinear(sampler, pos));
						EXPECT_EQ(base, sampler.position());
						EXPECT_EQ(bounds.containsPoint(base), sampler.currentPositionValid());
					}
				}
			}
		};
		RawVolume::Sampler sampler(volume);
		compare(sampler, volume.region());
		const Region selected(-1, -1, -1, 1, 2, 3);
		RawVolumeWrapper wrapper(&volume, selected);
		RawVolumeWrapper::Sampler wrapped(wrapper);
		compare(wrapped, selected);
		if (volume.isContiguous()) {
			RawVolume::DenseSampler dense(volume);
			compare(dense, volume.region());
		}
	}
}

class CornerCountingSampler : public RawVolume::Sampler {
	using Super = RawVolume::Sampler;
public:
	explicit CornerCountingSampler(RawVolume &volume) : Super(volume) {}
	mutable int reads = 0;
#define COUNT_CORNER(Name) const Voxel &Name() const { ++reads; return Super::Name(); }
	COUNT_CORNER(peekVoxel0px0py0pz)
	COUNT_CORNER(peekVoxel1px0py0pz)
	COUNT_CORNER(peekVoxel0px1py0pz)
	COUNT_CORNER(peekVoxel1px1py0pz)
	COUNT_CORNER(peekVoxel0px0py1pz)
	COUNT_CORNER(peekVoxel1px0py1pz)
	COUNT_CORNER(peekVoxel0px1py1pz)
	COUNT_CORNER(peekVoxel1px1py1pz)
#undef COUNT_CORNER
};

TEST_F(VolumeSamplerTest, TrilinearReadsOnlyTheSelectedCorner) {
	RawVolume volume(Region(-2, 3));
	volume.fill(Voxel(VoxelType::Generic, 17, 9, FlagOutline, 7));
	CornerCountingSampler sampler(volume);
	for (const glm::vec3 pos : {glm::vec3(0.25f), glm::vec3(-0.5f), glm::vec3(0.5f), glm::vec3(3.0f), glm::vec3(10.0f)}) {
		const glm::ivec3 base = glm::ivec3(glm::round(pos));
		const Voxel expected = volume.region().containsPoint(base) ? volume.voxel(base) : Voxel();
		sampler.reads = 0;
		EXPECT_EQ(expected, sampleTrilinear(sampler, pos));
		EXPECT_EQ(sampler.currentPositionValid() ? 1 : 0, sampler.reads);
	}
}

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

TEST_F(VolumeSamplerTest, testSampleNearestExactHit) {
	voxel::RawVolume v({0, 4});
	v.setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 7));
	voxel::VolumeSampler sampler(&v);
	EXPECT_EQ(7, sampleNearest(sampler, {2.0f, 2.0f, 2.0f}).getColor());
	EXPECT_EQ(7, sampleNearest(sampler, {2.4f, 1.6f, 2.1f}).getColor());
}

TEST_F(VolumeSamplerTest, testSampleNearestFillsGapFromNeighbor) {
	// Rounding lands on air next to a solid; nearest must pull the solid in.
	voxel::RawVolume v({0, 4});
	v.setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 9));
	voxel::VolumeSampler sampler(&v);
	const voxel::Voxel result = sampleNearest(sampler, {2.6f, 2.0f, 2.0f});
	EXPECT_EQ(9, result.getColor());
}

TEST_F(VolumeSamplerTest, testSampleNearestPicksCloserNeighbor) {
	voxel::RawVolume v({0, 5});
	v.setVoxel(1, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	v.setVoxel(3, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	voxel::VolumeSampler sampler(&v);
	// round(1.6)=2 (air); neighbor at x=1 is closer than x=3
	EXPECT_EQ(1, sampleNearest(sampler, {1.6f, 2.0f, 2.0f}).getColor());
	// round(2.4)=2 (air); neighbor at x=3 is closer than x=1
	EXPECT_EQ(2, sampleNearest(sampler, {2.4f, 2.0f, 2.0f}).getColor());
}

TEST_F(VolumeSamplerTest, testSampleNearestEmptyIsAir) {
	voxel::RawVolume v({0, 3});
	voxel::VolumeSampler sampler(&v);
	EXPECT_TRUE(voxel::isAir(sampleNearest(sampler, {1.5f, 1.5f, 1.5f}).getMaterial()));
}

} // namespace voxel
