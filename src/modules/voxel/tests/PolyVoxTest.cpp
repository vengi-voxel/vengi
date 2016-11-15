/**
 * @file
 */

#include "AbstractVoxelTest.h"

namespace voxel {

class PolyVoxTest: public AbstractVoxelTest {
protected:
	bool pageIn(const voxel::Region& region, PagedVolume::Chunk* chunk) override {
		chunk->setVoxel(1, 2, 1, createVoxel(VoxelType::Grass1));

		chunk->setVoxel(0, 1, 0, createVoxel(VoxelType::Dirt1));
		chunk->setVoxel(1, 1, 0, createVoxel(VoxelType::Dirt2));
		chunk->setVoxel(2, 1, 0, createVoxel(VoxelType::Dirt3));
		chunk->setVoxel(0, 1, 1, createVoxel(VoxelType::Dirt4));
		chunk->setVoxel(1, 1, 1, createVoxel(VoxelType::Leaves1));
		chunk->setVoxel(2, 1, 1, createVoxel(VoxelType::Leaves2));
		chunk->setVoxel(0, 1, 2, createVoxel(VoxelType::Leaves3));
		chunk->setVoxel(1, 1, 2, createVoxel(VoxelType::Leaves4));
		chunk->setVoxel(2, 1, 2, createVoxel(VoxelType::Leaves5));

		chunk->setVoxel(0, 0, 0, createVoxel(VoxelType::Leaves6));
		chunk->setVoxel(1, 0, 0, createVoxel(VoxelType::Leaves7));
		chunk->setVoxel(2, 0, 0, createVoxel(VoxelType::Leaves8));
		chunk->setVoxel(0, 0, 1, createVoxel(VoxelType::Leaves9));
		chunk->setVoxel(1, 0, 1, createVoxel(VoxelType::Leaves10));
		chunk->setVoxel(2, 0, 1, createVoxel(VoxelType::Rock1));
		chunk->setVoxel(0, 0, 2, createVoxel(VoxelType::Rock2));
		chunk->setVoxel(1, 0, 2, createVoxel(VoxelType::Rock3));
		chunk->setVoxel(2, 0, 2, createVoxel(VoxelType::Rock4));
		return true;
	}
};

TEST_F(PolyVoxTest, testSamplerPeek) {
	PagedVolume::Chunk* chunk = _volData.getChunk(glm::ivec3(0, 0, 0));
	ASSERT_EQ(VoxelType::Grass1, chunk->getVoxel(1, 2, 1).getMaterial());
	ASSERT_EQ(VoxelType::Leaves1, chunk->getVoxel(1, 1, 1).getMaterial());
	ASSERT_EQ(VoxelType::Leaves10, chunk->getVoxel(1, 0, 1).getMaterial());

	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 1, 1);
	ASSERT_EQ(VoxelType::Grass1, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different ";
	ASSERT_EQ(VoxelType::Leaves1, sampler.getVoxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Leaves10, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testSamplerPeekWithMovingX) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(0, 1, 1);
	sampler.movePositiveX();
	ASSERT_EQ(VoxelType::Grass1, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different ";
	ASSERT_EQ(VoxelType::Leaves1, sampler.getVoxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Leaves10, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testSamplerPeekWithAir) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 3, 1);
	ASSERT_EQ(VoxelType::Air, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different material";
	ASSERT_EQ(VoxelType::Air, sampler.getVoxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Grass1, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testSamplerPeekWithTipOfTheGeom) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 2, 1);
	ASSERT_EQ(VoxelType::Air, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different material";
	ASSERT_EQ(VoxelType::Grass1, sampler.getVoxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Leaves1, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testFullSamplerLoop) {
	const voxel::Region& region = _ctx.getRegion();
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
					if (x == 0 && z == 0) {
						ASSERT_EQ(VoxelType::Air, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves7, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves9, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves10, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Dirt1, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt2, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt4, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves1, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Leaves9, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Rock1, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Rock3, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves7, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves6, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves8, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Rock2, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Rock4, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Leaves1, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt4, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves2, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves4, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt2, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt1, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt3, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves3, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves5, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 1) {
					if (x == 0 && z == 0) {
						ASSERT_EQ(VoxelType::Air, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt2, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt4, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves1, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Grass1, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Leaves6, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Dirt4, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves2, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves4, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt2, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt1, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt3, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves3, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaves5, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Grass1, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Leaves10, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 2) {
					// 25
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Grass1, voxelCurrent.getMaterial()) << "Wrong voxel at coordinate " << x << ":" << y << ":" << z;
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
	voxel::Region region(mins, maxs);
	ASSERT_TRUE(region.containsPoint(mins));
	ASSERT_TRUE(region.containsPoint(maxs));
	ASSERT_FALSE(region.containsPoint(mins, 1));
	ASSERT_FALSE(region.containsPoint(maxs, 1));
	ASSERT_FALSE(region.containsPoint(maxs + 1));
	ASSERT_TRUE(region.containsRegion(region));
	ASSERT_FALSE(region.containsRegion(region, 1));
}

}
