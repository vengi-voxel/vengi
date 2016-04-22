#include "core/tests/AbstractTest.h"
#include "voxel/WorldPersister.h"
#include "voxel/polyvox/PagedVolume.h"
#include "voxel/Voxel.h"

namespace voxel {

class WorldPersisterTest: public core::AbstractTest {
};

class Pager: public PagedVolume::Pager {
public:
	void pageIn(const Region& region, PagedVolume::Chunk* chunk) override {
		const glm::ivec3 center(region.getWidthInVoxels() / 2, region.getHeightInVoxels() / 2, region.getDepthInVoxels() / 2);
		for (int z = 0; z < region.getDepthInVoxels(); ++z) {
			for (int y = 0; y < region.getHeightInVoxels(); ++y) {
				for (int x = 0; x < region.getWidthInVoxels(); ++x) {
					const glm::ivec3 pos(x, y, z);
					const int distance = (pos - center).length();
					Voxel uVoxelValue = createVoxel(Air);
					if (distance <= 30) {
						uVoxelValue = createVoxel(Grass);
					}

					chunk->setVoxel(x, y, z, uVoxelValue);
				}
			}
		}
	}

	void pageOut(const Region& region, PagedVolume::Chunk* chunk) override {
	}
};

TEST_F(WorldPersisterTest, testSaveLoad) {
	const voxel::Region region(glm::ivec3(0, 0, 0), glm::ivec3(63, 63, 63));
	Pager pager;
	PagedVolume volData(&pager, 256 * 1024 * 1024, 64);
	WorldPersister persister;
	TerrainContext ctx;
	ctx.region = region;
	ctx.chunk = volData.getChunk(region.getLowerCorner());
	ASSERT_TRUE(ctx.chunk != nullptr) << "Could not get chunk";
	ASSERT_TRUE(persister.save(ctx, 0)) << "Could not save volume chunk";
	const PagedVolume::Chunk* chunk = ctx.chunk;
	ctx.chunk = volData.getChunk(glm::ivec3(128, 0, 128));
	ASSERT_TRUE(chunk != ctx.chunk) << "Chunks should be different";
	ASSERT_TRUE(persister.load(ctx, 0)) << "Could not load volume chunk";
	ASSERT_EQ(Grass, ctx.chunk->getVoxel(32, 32, 32).getMaterial());
}

}
