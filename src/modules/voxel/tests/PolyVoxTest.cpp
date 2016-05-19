/**
 * @file
 */

#include "AbstractVoxelTest.h"

namespace voxel {

class PolyVoxTest: public AbstractVoxelTest {
protected:
	bool pageIn(const Region& region, PagedVolume::Chunk* chunk) override {
		chunk->setVoxel(1, 2, 1, createVoxel(25));

		chunk->setVoxel(0, 1, 0, createVoxel(11));
		chunk->setVoxel(1, 1, 0, createVoxel(12));
		chunk->setVoxel(2, 1, 0, createVoxel(13));
		chunk->setVoxel(0, 1, 1, createVoxel(14));
		chunk->setVoxel(1, 1, 1, createVoxel(15));
		chunk->setVoxel(2, 1, 1, createVoxel(16));
		chunk->setVoxel(0, 1, 2, createVoxel(17));
		chunk->setVoxel(1, 1, 2, createVoxel(18));
		chunk->setVoxel(2, 1, 2, createVoxel(19));

		chunk->setVoxel(0, 0, 0, createVoxel(1));
		chunk->setVoxel(1, 0, 0, createVoxel(2));
		chunk->setVoxel(2, 0, 0, createVoxel(3));
		chunk->setVoxel(0, 0, 1, createVoxel(4));
		chunk->setVoxel(1, 0, 1, createVoxel(5));
		chunk->setVoxel(2, 0, 1, createVoxel(6));
		chunk->setVoxel(0, 0, 2, createVoxel(7));
		chunk->setVoxel(1, 0, 2, createVoxel(8));
		chunk->setVoxel(2, 0, 2, createVoxel(9));
		return true;
	}
};

TEST_F(PolyVoxTest, testSamplerPeek) {
	PagedVolume::Chunk* chunk = _volData.getChunk(glm::ivec3(0, 0, 0));
	ASSERT_EQ(25, chunk->getVoxel(1, 2, 1).getMaterial());
	ASSERT_EQ(15, chunk->getVoxel(1, 1, 1).getMaterial());
	ASSERT_EQ(5, chunk->getVoxel(1, 0, 1).getMaterial());

	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 1, 1);
	ASSERT_EQ(25, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different ";
	ASSERT_EQ(15, sampler.getVoxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(5, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testSamplerPeekWithMovingX) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(0, 1, 1);
	sampler.movePositiveX();
	ASSERT_EQ(25, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different ";
	ASSERT_EQ(15, sampler.getVoxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(5, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testSamplerPeekWithAir) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 3, 1);
	ASSERT_EQ(0, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different material";
	ASSERT_EQ(0, sampler.getVoxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(25, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testSamplerPeekWithTipOfTheGeom) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 2, 1);
	ASSERT_EQ(0, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different material";
	ASSERT_EQ(25, sampler.getVoxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(15, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testFullSamplerLoop) {
	const Region& region = _ctx.region;
	PagedVolume::Sampler volumeSampler(&_volData);

	ASSERT_EQ(0, region.getLowerX());
	ASSERT_EQ(0, region.getLowerY());
	ASSERT_EQ(0, region.getLowerZ());

	for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z++) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y++) {
			volumeSampler.setPosition(region.getLowerX(), y, z);

			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x++) {
				const Voxel voxelCurrent          = volumeSampler.getVoxel();
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
					// 1 - 9
					if (x < 3 && z < 3) {
						const int expected = 1 + x + z * 3;
						ASSERT_EQ(expected, voxelCurrent.getMaterial()) << "Wrong voxel at coordinate " << x << ":" << y << ":" << z;
					}
					if (x == 0 && z == 0) {
						ASSERT_EQ(0, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(2, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(4, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(0, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(0, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(0, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(0, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(5, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(11, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(12, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(14, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(15, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(0, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(4, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(6, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(8, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(2, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(1, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(3, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(7, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(9, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(15, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(14, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(16, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(18, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(12, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(11, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(13, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(17, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(19, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(0, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 1) {
					if (x < 3 && z < 3) {
						const int expected = 11 + x + z * 3;
						ASSERT_EQ(expected, voxelCurrent.getMaterial()) << "Wrong voxel at coordinate " << x << ":" << y << ":" << z;
					}
					if (x == 0 && z == 0) {
						ASSERT_EQ( 0, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(12, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(14, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(15, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ( 0, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(25, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(1, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(14, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(16, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(18, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(12, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(11, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(13, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(17, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(19, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(25, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ( 0, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(5, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 2) {
					// 25
					if (x == 1 && z == 1) {
						ASSERT_EQ(25, voxelCurrent.getMaterial()) << "Wrong voxel at coordinate " << x << ":" << y << ":" << z;
					}
				}

				volumeSampler.movePositiveX();
			}
		}
	}
}

TEST_F(PolyVoxTest, testRegion) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(15, 15, 15);
	Region region(mins, maxs);
	ASSERT_TRUE(region.containsPoint(mins));
	ASSERT_TRUE(region.containsPoint(maxs));
	ASSERT_FALSE(region.containsPoint(mins, 1));
	ASSERT_FALSE(region.containsPoint(maxs, 1));
	ASSERT_FALSE(region.containsPoint(maxs + 1));
	ASSERT_TRUE(region.containsRegion(region));
	ASSERT_FALSE(region.containsRegion(region, 1));
}

}
