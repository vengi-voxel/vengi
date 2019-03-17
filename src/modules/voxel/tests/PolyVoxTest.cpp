/**
 * @file
 */

#include "AbstractVoxelTest.h"

namespace voxel {

class PolyVoxTest: public AbstractVoxelTest {
protected:
	bool pageIn(const voxel::Region& region, const PagedVolume::ChunkPtr& chunk) override {
		chunk->setVoxel(1, 2, 1, createVoxel(VoxelType::Grass, 0));

		chunk->setVoxel(0, 1, 0, createVoxel(VoxelType::Dirt, 0));
		chunk->setVoxel(1, 1, 0, createVoxel(VoxelType::Dirt, 0));
		chunk->setVoxel(2, 1, 0, createVoxel(VoxelType::Dirt, 0));
		chunk->setVoxel(0, 1, 1, createVoxel(VoxelType::Dirt, 0));
		chunk->setVoxel(1, 1, 1, createVoxel(VoxelType::Leaf, 0));
		chunk->setVoxel(2, 1, 1, createVoxel(VoxelType::Leaf, 0));
		chunk->setVoxel(0, 1, 2, createVoxel(VoxelType::Leaf, 0));
		chunk->setVoxel(1, 1, 2, createVoxel(VoxelType::Leaf, 0));
		chunk->setVoxel(2, 1, 2, createVoxel(VoxelType::Leaf, 0));

		chunk->setVoxel(0, 0, 0, createVoxel(VoxelType::Leaf, 0));
		chunk->setVoxel(1, 0, 0, createVoxel(VoxelType::Leaf, 0));
		chunk->setVoxel(2, 0, 0, createVoxel(VoxelType::Leaf, 0));
		chunk->setVoxel(0, 0, 1, createVoxel(VoxelType::Leaf, 0));
		chunk->setVoxel(1, 0, 1, createVoxel(VoxelType::Leaf, 0));
		chunk->setVoxel(2, 0, 1, createVoxel(VoxelType::Rock, 0));
		chunk->setVoxel(0, 0, 2, createVoxel(VoxelType::Rock, 0));
		chunk->setVoxel(1, 0, 2, createVoxel(VoxelType::Rock, 0));
		chunk->setVoxel(2, 0, 2, createVoxel(VoxelType::Rock, 0));
		return true;
	}
};

TEST_F(PolyVoxTest, testSamplerPeek) {
	const PagedVolume::ChunkPtr& chunk = _volData.chunk(glm::ivec3(0, 0, 0));
	ASSERT_EQ(VoxelType::Grass, chunk->voxel(1, 2, 1).getMaterial());
	ASSERT_EQ(VoxelType::Leaf, chunk->voxel(1, 1, 1).getMaterial());
	ASSERT_EQ(VoxelType::Leaf, chunk->voxel(1, 0, 1).getMaterial());

	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 1, 1);
	ASSERT_EQ(VoxelType::Grass, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different ";
	ASSERT_EQ(VoxelType::Leaf, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Leaf, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testSamplerPeekWithMovingX) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(0, 1, 1);
	sampler.movePositiveX();
	ASSERT_EQ(VoxelType::Grass, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different ";
	ASSERT_EQ(VoxelType::Leaf, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Leaf, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testSamplerPeekWithAir) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 3, 1);
	ASSERT_EQ(VoxelType::Air, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different material";
	ASSERT_EQ(VoxelType::Air, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Grass, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testSamplerPeekWithTipOfTheGeom) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 2, 1);
	ASSERT_EQ(VoxelType::Air, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different material";
	ASSERT_EQ(VoxelType::Grass, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Leaf, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testFullSamplerLoop) {
	const voxel::Region& region = _ctx.region();
	PagedVolume::Sampler volumeSampler(&_volData);

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
						ASSERT_EQ(VoxelType::Leaf, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Dirt, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Leaf, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Rock, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Rock, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Rock, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Rock, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Leaf, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 1) {
					if (x == 0 && z == 0) {
						ASSERT_EQ(VoxelType::Air, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Grass, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Leaf, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Dirt, voxelLeft.getMaterial()) << "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelRight.getMaterial()) << "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelBehind.getMaterial()) << "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt, voxelBefore.getMaterial()) << "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt, voxelLeftBefore.getMaterial()) << "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Dirt, voxelRightBefore.getMaterial()) << "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelLeftBehind.getMaterial()) << "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Leaf, voxelRightBehind.getMaterial()) << "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Grass, voxelAbove.getMaterial()) << "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial()) << "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRight.getMaterial()) << "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial()) << "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBehind.getMaterial()) << "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Leaf, voxelBelow.getMaterial()) << "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 2) {
					// 25
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Grass, voxelCurrent.getMaterial()) << "Wrong voxel at coordinate " << x << ":" << y << ":" << z;
					}
				}

				volumeSampler.movePositiveX();
			}
		}
	}
}

}
