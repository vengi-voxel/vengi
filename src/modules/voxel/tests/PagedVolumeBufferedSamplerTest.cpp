/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/PagedVolume.h"

namespace voxel {

class PagedVolumeBufferedSamplerTest: public AbstractVoxelTest {
};

TEST_F(PagedVolumeBufferedSamplerTest, testCenterPosition) {
	const Region region(glm::ivec3(16), glm::ivec3(31));
	PagedVolume::BufferedSampler sampler(_volData, region);
	ASSERT_TRUE(sampler.setPosition(region.getCentre()));
}

TEST_F(PagedVolumeBufferedSamplerTest, testExtractWhole) {
	PagedVolume::BufferedSampler sampler(_volData, _region);
	ASSERT_TRUE(sampler.setPosition(_region.getCentre()));
}

TEST_F(PagedVolumeBufferedSamplerTest, testPeekUpperCorner) {
	PagedVolume::BufferedSampler sampler(_volData, _region);
	ASSERT_TRUE(sampler.setPosition(_region.getUpperCorner()));
	sampler.peekVoxel1nx1ny1nz();
	sampler.peekVoxel1px1py1pz();
}

}
