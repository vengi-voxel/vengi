/**
 * @file
 */

#include "AbstractVoxelTest.h"

namespace voxel {

class PolyVoxTest: public AbstractVoxelTest {
protected:
	bool pageIn(const voxel::Region& region, const PagedVolume::ChunkPtr& chunk) override {
		chunk->setVoxel(1, 2, 1, createVoxel(VoxelType::Generic, 0));

		chunk->setVoxel(0, 1, 0, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(1, 1, 0, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(2, 1, 0, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(0, 1, 1, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(2, 1, 1, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(0, 1, 2, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(1, 1, 2, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(2, 1, 2, createVoxel(VoxelType::Generic, 0));

		chunk->setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(1, 0, 0, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(2, 0, 0, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(0, 0, 1, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(1, 0, 1, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(2, 0, 1, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(0, 0, 2, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(1, 0, 2, createVoxel(VoxelType::Generic, 0));
		chunk->setVoxel(2, 0, 2, createVoxel(VoxelType::Generic, 0));
		return true;
	}
};

TEST_F(PolyVoxTest, testSamplerPeek) {
	const PagedVolume::ChunkPtr& chunk = _volData.chunk(glm::ivec3(0, 0, 0));
	ASSERT_EQ(VoxelType::Generic, chunk->voxel(1, 2, 1).getMaterial());
	ASSERT_EQ(VoxelType::Generic, chunk->voxel(1, 1, 1).getMaterial());
	ASSERT_EQ(VoxelType::Generic, chunk->voxel(1, 0, 1).getMaterial());

	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 1, 1);
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different ";
	ASSERT_EQ(VoxelType::Generic, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testChunkPos) {
	const int length = _volData.chunkSideLength();
	EXPECT_EQ(64, length);
	const int half = length / 2;
	EXPECT_EQ(_volData.chunkPos(half, half, half), glm::ivec3(0, 0, 0));
	EXPECT_EQ(_volData.chunkPos(half - 1, half - 1, half - 1), glm::ivec3(0, 0, 0));
	EXPECT_EQ(_volData.chunkPos(length - 1, length - 1, length - 1), glm::ivec3(0, 0, 0));
	EXPECT_EQ(_volData.chunkPos(0, 0, 0), glm::ivec3(0, 0, 0));
	EXPECT_EQ(_volData.chunkPos(1, 1, 1), glm::ivec3(0, 0, 0));
	EXPECT_EQ(_volData.chunkPos(-1, -1, -1), glm::ivec3(-1, -1, -1));
	EXPECT_EQ(_volData.chunkPos(-(length + 1), -(length + 1), -(length + 1)), glm::ivec3(-2, -2, -2));
	EXPECT_EQ(_volData.chunkPos(-length, -length, -length), glm::ivec3(-1, -1, -1));
	EXPECT_EQ(_volData.chunkPos(-(length - 1), -(length - 1), -(length - 1)), glm::ivec3(-1, -1, -1));
	EXPECT_EQ(_volData.chunkPos(length, length, length), glm::ivec3(1, 1, 1));
}

TEST_F(PolyVoxTest, testSamplerPeekWithMovingX) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(0, 1, 1);
	sampler.movePositiveX();
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different ";
	ASSERT_EQ(VoxelType::Generic, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testSamplerPeekWithAir) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 3, 1);
	ASSERT_EQ(VoxelType::Air, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different material";
	ASSERT_EQ(VoxelType::Air, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
}

TEST_F(PolyVoxTest, testSamplerPeekWithTipOfTheGeom) {
	PagedVolume::Sampler sampler(&_volData);
	sampler.setPosition(1, 2, 1);
	ASSERT_EQ(VoxelType::Air, sampler.peekVoxel0px1py0pz().getMaterial()) << "The voxel above the current position should have a different material";
	ASSERT_EQ(VoxelType::Generic, sampler.voxel().getMaterial()) << "The current voxel should have a different material";
	ASSERT_EQ(VoxelType::Generic, sampler.peekVoxel0px1ny0pz().getMaterial()) << "The voxel below the current position should have a different ";
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
