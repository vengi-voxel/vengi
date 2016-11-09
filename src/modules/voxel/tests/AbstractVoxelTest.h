/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "voxel/polyvox/PagedVolume.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/WorldContext.h"
#include "voxel/Constants.h"
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

		bool pageIn(PagedVolume::PagerContext& ctx) override {
			return _test->pageIn(ctx.region, ctx.chunk);
		}

		void pageOut(PagedVolume::PagerContext& ctx) override {
		}
	};

	virtual bool pageIn(const Region& region, PagedVolume::Chunk* chunk) {
		const glm::vec3 center(region.getCentre());
		for (int z = 0; z < region.getDepthInVoxels(); ++z) {
			for (int y = 0; y < region.getHeightInVoxels(); ++y) {
				for (int x = 0; x < region.getWidthInVoxels(); ++x) {
					const glm::vec3 pos(x, y, z);
					const float distance = glm::distance(pos, center);
					Voxel uVoxelValue = createVoxel(VoxelType::Air);
					if (distance <= 30.0f) {
						uVoxelValue = createVoxel(VoxelType::Grass1);
					}

					chunk->setVoxel(x, y, z, uVoxelValue);
				}
			}
		}
		return true;
	}

	inline std::string str(const voxel::Region& region) const {
		return "mins(" + glm::to_string(region.getLowerCorner()) + "), maxs(" + glm::to_string(region.getUpperCorner()) + ")";
	}

	Pager _pager;
	PagedVolume _volData;
	GeneratorContext _ctx;
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
