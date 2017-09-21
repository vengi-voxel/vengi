/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "voxel/polyvox/PagedVolumeWrapper.h"
#include "voxel/polyvox/PagedVolume.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/WorldContext.h"
#include "voxel/MaterialColor.h"
#include "voxel/Constants.h"
#include "core/Random.h"
#include "core/Common.h"

namespace voxel {

inline bool operator==(const voxel::RawVolume& volume1, const voxel::RawVolume& volume2) {
	for (int i = 0; i < 3; ++i) {
		if (volume1.mins()[i] != volume2.mins()[i]) {
			return false;
		}
		if (volume1.maxs()[i] != volume2.maxs()[i]) {
			return false;
		}
	}

	const voxel::Region& region = volume1.region();
	const int32_t lowerX = region.getLowerX();
	const int32_t lowerY = region.getLowerY();
	const int32_t lowerZ = region.getLowerZ();
	const int32_t upperX = region.getUpperX();
	const int32_t upperY = region.getUpperY();
	const int32_t upperZ = region.getUpperZ();
	for (int32_t z = lowerZ; z <= upperZ; ++z) {
		for (int32_t y = lowerY; y <= upperY; ++y) {
			for (int32_t x = lowerX; x <= upperX; ++x) {
				const glm::ivec3 pos(x, y, z);
				const voxel::Voxel& voxel1 = volume1.voxel(pos);
				const voxel::Voxel& voxel2 = volume2.voxel(pos);
				if (!voxel1.isSame(voxel2)) {
					return false;
				}
			}
		}
	}
	return true;
}

inline ::std::ostream& operator<<(::std::ostream& os, const voxel::Region& region) {
	return os << "region["
			<< "center(" << glm::to_string(region.getCentre()) << "), "
			<< "mins(" << glm::to_string(region.getLowerCorner()) << "), "
			<< "maxs(" << glm::to_string(region.getUpperCorner()) << ")"
			<< "]";
}

inline ::std::ostream& operator<<(::std::ostream& os, const voxel::Voxel& voxel) {
	return os << "voxel[" << voxel::VoxelTypeStr[(int)voxel.getMaterial()] << ", " << (int)voxel.getColor() << "]";
}

inline ::std::ostream& operator<<(::std::ostream& os, const voxel::RawVolume& volume) {
	const voxel::Region& region = volume.region();
	os << "volume[" << region;
	const int threshold = 6;
	if (volume.depth() <= threshold && volume.width() <= threshold && volume.height() <= threshold) {
		const int32_t lowerX = region.getLowerX();
		const int32_t lowerY = region.getLowerY();
		const int32_t lowerZ = region.getLowerZ();
		const int32_t upperX = region.getUpperX();
		const int32_t upperY = region.getUpperY();
		const int32_t upperZ = region.getUpperZ();
		os << "\n";
		for (int32_t z = lowerZ; z <= upperZ; ++z) {
			for (int32_t y = lowerY; y <= upperY; ++y) {
				for (int32_t x = lowerX; x <= upperX; ++x) {
					const glm::ivec3 pos(x, y, z);
					const voxel::Voxel& voxel = volume.voxel(pos);
					os << x << ", " << y << ", " << z << ": " << voxel << "\n";
				}
			}
		}
	}
	os << "]";
	return os;
}

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

		void pageOut(PagedVolume::Chunk* chunk) override {
		}
	};

	virtual bool pageIn(const Region& region, const PagedVolume::ChunkPtr& chunk) {
		const glm::vec3 center(region.getCentre());
		for (int z = 0; z < region.getDepthInVoxels(); ++z) {
			for (int y = 0; y < region.getHeightInVoxels(); ++y) {
				for (int x = 0; x < region.getWidthInVoxels(); ++x) {
					const glm::vec3 pos(x, y, z);
					const float distance = glm::distance(pos, center);
					Voxel uVoxelValue;
					if (distance <= 30.0f) {
						uVoxelValue = createRandomColorVoxel(VoxelType::Grass);
					}

					chunk->setVoxel(x, y, z, uVoxelValue);
				}
			}
		}
		return true;
	}

	Pager _pager;
	PagedVolume _volData;
	PagedVolumeWrapper _ctx;
	core::Random _random;
	long _seed = 0;
	const voxel::Region _region { glm::ivec3(0), glm::ivec3(63) };

	AbstractVoxelTest() :
			_pager(this), _volData(&_pager, 128 * 1024 * 1024, 64), _ctx(nullptr, nullptr, voxel::Region()) {
	}

public:
	void SetUp() override {
		_volData.flushAll();
		core::AbstractTest::SetUp();
		ASSERT_TRUE(voxel::initDefaultMaterialColors());
		_random.setSeed(_seed);
		_ctx = PagedVolumeWrapper(&_volData, _volData.chunk(_region.getCentre()), _region);
	}
};

}
