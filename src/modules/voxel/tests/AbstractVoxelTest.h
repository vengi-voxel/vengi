/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "voxel/polyvox/PagedVolume.h"
#include "voxel/WorldContext.h"
#include "voxel/Voxel.h"
#include "core/Random.h"

namespace voxel {

class AbstractVoxelTest: public core::AbstractTest {
protected:
	class Pager: public PagedVolume::Pager {
		AbstractVoxelTest* _test;
	public:
		Pager(AbstractVoxelTest* test) :
				_test(test) {
		}

		bool pageIn(const Region& region, PagedVolume::Chunk* chunk) override {
			return _test->pageIn(region, chunk);
		}

		void pageOut(const Region& region, PagedVolume::Chunk* chunk) override {
		}
	};

	virtual bool pageIn(const Region& region, PagedVolume::Chunk* chunk) {
		const glm::ivec3 center(region.getWidthInVoxels() / 2, region.getHeightInVoxels() / 2, region.getDepthInVoxels() / 2);
		for (int z = 0; z < region.getDepthInVoxels(); ++z) {
			for (int y = 0; y < region.getHeightInVoxels(); ++y) {
				for (int x = 0; x < region.getWidthInVoxels(); ++x) {
					const glm::ivec3 pos(x, y, z);
					const int distance = (pos - center).length();
					Voxel uVoxelValue = createVoxel(Air);
					if (distance <= 30) {
						uVoxelValue = createVoxel(Grass1);
					}

					chunk->setVoxel(x, y, z, uVoxelValue);
				}
			}
		}
		return true;
	}

	Pager _pager;
	PagedVolume _volData;
	TerrainContext _ctx;
	core::Random _random;
	long _seed = 0;

	AbstractVoxelTest() :
			_pager(this), _volData(&_pager, 16 * 1024 * 1024, 64), _ctx(&_volData, nullptr) {
	}

public:
	void SetUp() override {
		_volData.flushAll();
		core::AbstractTest::SetUp();
		_random.setSeed(_seed);
		const voxel::Region region(glm::ivec3(0, 0, 0), glm::ivec3(63, 63, 63));
		_ctx.region = region;
		_ctx.setChunk(_volData.getChunk(region.getCentre()));
	}
};

}
