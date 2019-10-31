/**
 * @file
 */

#pragma once

#include "core/tests/AbstractTest.h"
#include "voxel/PagedVolumeWrapper.h"
#include "voxel/PagedVolume.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/MaterialColor.h"
#include "voxel/Constants.h"
#include "math/Random.h"
#include "core/Common.h"

namespace voxel {

static int VolumePrintThreshold = 10;

inline bool operator==(const voxel::RawVolume& volume1, const voxel::RawVolume& volume2) {
	const Region& r1 = volume1.region();
	const Region& r2 = volume2.region();
	if (r1 != r2) {
		Log::debug("region differs");
		return false;
	}

	const voxel::Region& region = volume1.region();
	const int32_t lowerX = region.getLowerX();
	const int32_t lowerY = region.getLowerY();
	const int32_t lowerZ = region.getLowerZ();
	const int32_t upperX = region.getUpperX();
	const int32_t upperY = region.getUpperY();
	const int32_t upperZ = region.getUpperZ();
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();

	for (int32_t z = lowerZ; z <= upperZ; ++z) {
		for (int32_t y = lowerY; y <= upperY; ++y) {
			for (int32_t x = lowerX; x <= upperX; ++x) {
				const glm::ivec3 pos(x, y, z);
				const voxel::Voxel& voxel1 = volume1.voxel(pos);
				const voxel::Voxel& voxel2 = volume2.voxel(pos);
				if (voxel1.getMaterial() != voxel2.getMaterial()) {
					Log::error("Voxel differs at %i:%i:%i in material - voxel1[%s, %i], voxel2[%s, %i]", x, y, z,
							voxel::VoxelTypeStr[(int)voxel1.getMaterial()], (int)voxel1.getColor(), voxel::VoxelTypeStr[(int)voxel2.getMaterial()], (int)voxel2.getColor());
					return false;
				}
				const glm::vec4& c1 = materialColors[voxel1.getColor()];
				const glm::vec4& c2 = materialColors[voxel2.getColor()];
				const glm::vec4& delta = c1 - c2;
				if (glm::any(glm::greaterThan(delta, glm::vec4(glm::epsilon<float>())))) {
					Log::error("Voxel differs at %i:%i:%i in color - voxel1[%s, %i], voxel2[%s, %i]", x, y, z,
							voxel::VoxelTypeStr[(int)voxel1.getMaterial()], (int)voxel1.getColor(), voxel::VoxelTypeStr[(int)voxel2.getMaterial()], (int)voxel2.getColor());
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
	if (volume.depth() <= VolumePrintThreshold && volume.width() <= VolumePrintThreshold && volume.height() <= VolumePrintThreshold) {
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
	math::Random _random;
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
		VolumePrintThreshold = 10;
	}
};

}
