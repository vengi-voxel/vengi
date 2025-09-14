/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "core/collection/Buffer.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxel {

int countVoxels(const voxel::RawVolume &volume) {
	int cnt = 0;
	for (int z = 0; z < volume.depth(); ++z) {
		for (int y = 0; y < volume.height(); ++y) {
			for (int x = 0; x < volume.width(); ++x) {
				if (!voxel::isAir(volume.voxel(x, y, z).getMaterial())) {
					++cnt;
				}
			}
		}
	}
	return cnt;
}

class RawVolumeTest: public AbstractVoxelTest {
protected:
	bool pageIn(const voxel::Region& region, RawVolume &v) {
		v.setVoxel(1, 2, 1, voxel::createVoxel(VoxelType::Generic, 0));

		v.setVoxel(0, 1, 0, voxel::createVoxel(VoxelType::Generic, 0));
		v.setVoxel(1, 1, 0, voxel::createVoxel(VoxelType::Generic, 0));
		v.setVoxel(2, 1, 0, voxel::createVoxel(VoxelType::Generic, 0));
		v.setVoxel(0, 1, 1, voxel::createVoxel(VoxelType::Generic, 0));
		v.setVoxel(1, 1, 1, voxel::createVoxel(VoxelType::Generic, 0));
		v.setVoxel(2, 1, 1, voxel::createVoxel(VoxelType::Generic, 0));
		v.setVoxel(0, 1, 2, voxel::createVoxel(VoxelType::Generic, 0));
		v.setVoxel(1, 1, 2, voxel::createVoxel(VoxelType::Generic, 0));
		v.setVoxel(2, 1, 2, voxel::createVoxel(VoxelType::Generic, 0));

		v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Generic, 1));
		v.setVoxel(1, 0, 0, voxel::createVoxel(VoxelType::Generic, 2));
		v.setVoxel(2, 0, 0, voxel::createVoxel(VoxelType::Generic, 3));
		v.setVoxel(0, 0, 1, voxel::createVoxel(VoxelType::Generic, 4));
		v.setVoxel(1, 0, 1, voxel::createVoxel(VoxelType::Generic, 5));
		v.setVoxel(2, 0, 1, voxel::createVoxel(VoxelType::Generic, 6));
		v.setVoxel(0, 0, 2, voxel::createVoxel(VoxelType::Generic, 7));
		v.setVoxel(1, 0, 2, voxel::createVoxel(VoxelType::Generic, 8));
		v.setVoxel(2, 0, 2, voxel::createVoxel(VoxelType::Generic, 9));
		return true;
	}
};

TEST_F(RawVolumeTest, testIsEmpty) {
	RawVolume v(_region);
	pageIn(v.region(), v);
	ASSERT_EQ(VoxelType::Generic, v.voxel(1, 2, 1).getMaterial());
	ASSERT_EQ(VoxelType::Generic, v.voxel(1, 1, 1).getMaterial());
	ASSERT_EQ(VoxelType::Generic, v.voxel(1, 0, 1).getMaterial());
	ASSERT_FALSE(v.isEmpty(v.region()));
	ASSERT_FALSE(v.isEmpty(Region{0, 2}));
	ASSERT_TRUE(v.isEmpty(Region{30, 63}));
}

TEST_F(RawVolumeTest, testCopyRegions) {
	RawVolume v(_region);
	pageIn(v.region(), v);

	core::Buffer<voxel::Region> regions;
	regions.push_back(Region(0, 0, 2, 0, 0, 2)); // color index 7
	regions.push_back(Region(2, 0, 0, 2, 0, 0)); // color index 3
	RawVolume v2(v, regions);
	EXPECT_EQ(v2.region(), Region(0, 0, 0, 2, 0, 2));
	EXPECT_EQ(7, v2.voxel(0, 0, 2).getColor());
	EXPECT_EQ(3, v2.voxel(2, 0, 0).getColor());
}

TEST_F(RawVolumeTest, testCopy) {
	RawVolume v(_region);
	pageIn(v.region(), v);

	RawVolume v2(v, Region{0, 2});
	EXPECT_EQ(19, countVoxels(v2));

	RawVolume v3(v, Region{3, 5});
	EXPECT_EQ(0, countVoxels(v3));
}

TEST_F(RawVolumeTest, testCopyInto) {
	RawVolume v(_region);
	pageIn(v.region(), v);

	RawVolume v2(_region);
	v2.setVoxel(1, 1, 1, voxel::createVoxel(VoxelType::Generic, 42));
	v.copyInto(v2, {1, 1});
	ASSERT_EQ(42, v.voxel(1, 1, 1).getColor());
}

TEST_F(RawVolumeTest, testSamplerPeek) {
	RawVolume v(_region);
	pageIn(v.region(), v);
	ASSERT_EQ(VoxelType::Generic, v.voxel(1, 2, 1).getMaterial());
	ASSERT_EQ(VoxelType::Generic, v.voxel(1, 1, 1).getMaterial());
	ASSERT_EQ(VoxelType::Generic, v.voxel(1, 0, 1).getMaterial());

	RawVolume::Sampler sampler(v);
	sampler.setPosition(1, 1, 1);
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different ";
	ASSERT_EQ(VoxelType::Generic, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(RawVolumeTest, testSamplerPeekWithMovingX) {
	RawVolume v(_region);
	pageIn(v.region(), v);
	RawVolume::Sampler sampler(v);
	sampler.setPosition(0, 1, 1);
	sampler.movePositiveX();
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different ";
	ASSERT_EQ(VoxelType::Generic, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(RawVolumeTest, testSamplerPeekWithAir) {
	RawVolume v(_region);
	pageIn(v.region(), v);
	RawVolume::Sampler sampler(v);
	sampler.setPosition(1, 3, 1);
	ASSERT_EQ(VoxelType::Air, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different material";
	ASSERT_EQ(VoxelType::Air, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(RawVolumeTest, testSamplerPeekWithTipOfTheGeom) {
	RawVolume v(_region);
	pageIn(v.region(), v);
	RawVolume::Sampler sampler(v);
	sampler.setPosition(1, 2, 1);
	ASSERT_EQ(VoxelType::Air, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different material";
	ASSERT_EQ(VoxelType::Generic, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(RawVolumeTest, testMove) {
	RawVolume v({glm::ivec3(0), glm::ivec3(1)});
	v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Generic, 1));
	v.move({1, 0, 0});
	EXPECT_EQ((int)v.voxel(0, 0, 0).getColor(), 0);
	EXPECT_EQ((int)v.voxel(0, 0, 0).getMaterial(), (int)VoxelType::Air);
	EXPECT_EQ((int)v.voxel(1, 0, 0).getColor(), 1);
	EXPECT_EQ((int)v.voxel(1, 0, 0).getMaterial(), (int)VoxelType::Generic);
}

TEST_F(RawVolumeTest, testFullSamplerLoop) {
	RawVolume v(_region);
	pageIn(v.region(), v);
	RawVolume::Sampler volumeSampler(v);

	const voxel::Region &region = v.region();
	ASSERT_EQ(0, region.getLowerX());
	ASSERT_EQ(0, region.getLowerY());
	ASSERT_EQ(0, region.getLowerZ());

	for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z++) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y++) {
			volumeSampler.setPosition(region.getLowerX(), y, z);

			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x++) {
				const Voxel voxelCurrent          = volumeSampler.voxel();
				const Voxel voxelLeft             = volumeSampler.peekVoxel1nx0py0pz();
				const Voxel voxelRight            = volumeSampler.peekVoxel1px0py0pz();
				const Voxel voxelBefore           = volumeSampler.peekVoxel0px0py1nz();
				const Voxel voxelBehind           = volumeSampler.peekVoxel0px0py1pz();
				const Voxel voxelLeftBefore       = volumeSampler.peekVoxel1nx0py1nz();
				const Voxel voxelRightBefore      = volumeSampler.peekVoxel1px0py1nz();
				const Voxel voxelLeftBehind       = volumeSampler.peekVoxel1nx0py1pz();
				const Voxel voxelRightBehind      = volumeSampler.peekVoxel1px0py1pz();

				const Voxel voxelAbove            = volumeSampler.peekVoxel0px1py0pz();
				const Voxel voxelAboveLeft        = volumeSampler.peekVoxel1nx1py0pz();
				const Voxel voxelAboveRight       = volumeSampler.peekVoxel1px1py0pz();
				const Voxel voxelAboveBefore      = volumeSampler.peekVoxel0px1py1nz();
				const Voxel voxelAboveBehind      = volumeSampler.peekVoxel0px1py1pz();
				const Voxel voxelAboveLeftBefore  = volumeSampler.peekVoxel1nx1py1nz();
				const Voxel voxelAboveRightBefore = volumeSampler.peekVoxel1px1py1nz();
				const Voxel voxelAboveLeftBehind  = volumeSampler.peekVoxel1nx1py1pz();
				const Voxel voxelAboveRightBehind = volumeSampler.peekVoxel1px1py1pz();

				const Voxel voxelBelow            = volumeSampler.peekVoxel0px1ny0pz();

				if (y == 0) {
					if (x == 0 && z == 0) {
						ASSERT_EQ(VoxelType::Air, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Generic, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 1) {
					if (x == 0 && z == 0) {
						ASSERT_EQ(VoxelType::Air, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Generic, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 2) {
					// 25
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Generic, voxelCurrent.getMaterial()) << "Wrong voxel at coordinate " << x << ":" << y << ":" << z;
					}
				}

				volumeSampler.movePositiveX();
			}
		}
	}
}

}
