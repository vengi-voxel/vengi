/**
 * @file
 */

#include "rma/MapBuilder.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "io/File.h"
#include "io/Filesystem.h"
#include "rma/MetaMap.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxelformat/VolumeCache.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeVisitor.h"
#include "gtest/gtest.h"

namespace rma {

static const Tile street_v_1{"0", "vx", "0", "a", "+v", "a", "0", "vx", "0"};
static const Tile street_h_1{"0", "a", "0", "hx", "+h", "hx", "0", "a", "0"};
static const Tile street_cross_1{"0", "vx", "0", "hx", "+x", "hx", "0", "vx", "0"};
static const Tile house1{"0", "ha", "0", "va", "+a", "va", "0", "ha", "0"};

class MapBuilderTest : public app::AbstractTest {
protected:
	voxelformat::VolumeCachePtr _volumeCache;

	bool onInitApp() override {
		voxel::initDefaultMaterialColors();
		_volumeCache = std::make_shared<voxelformat::VolumeCache>();
		if (!_volumeCache->init()) {
			return false;
		}
		return true;
	}

	void onCleanupApp() override {
		_volumeCache->shutdown();
	}

	bool save(const LevelVolumes &volumes) const {
		voxel::VoxelVolumes v;
		v.resize(volumes.size());
		int j = 0;
		for (int i = 0; i < (int)volumes.size(); ++i) {
			if (volumes[i] != nullptr) {
				v[j++] = volumes[i];
			}
		}
		v.resize(j);
		const io::FilePtr &file = _testApp->filesystem()->open("mapbuildertest.qb", io::FileMode::SysWrite);
		return voxelformat::saveFormat(file, v);
	}

	void cleanup(const LevelVolumes &volumes) {
		for (voxel::RawVolume *v : volumes) {
			delete v;
		}
	}

	bool empty(const rma::LevelVolumes &volumes) const {
		bool empty = true;
		for (voxel::RawVolume *v : volumes) {
			if (v == nullptr) {
				return true;
			}
			voxelutil::visitVolume(*v, [&empty](int, int, int, const voxel::Voxel &voxel) {
				if (!voxel::isAir(voxel.getMaterial())) {
					empty = false;
				}
			});
		}
		return empty;
	}

	MetaMap createMetaMap() const {
		MetaMap metaMap("test");
		metaMap.width = 3;
		metaMap.height = 3;
		metaMap.tiles.put("house1", house1);
		metaMap.tiles.put("street_h_1", street_h_1);
		metaMap.tiles.put("street_v_1", street_v_1);
		metaMap.tiles.put("street_cross_1", street_cross_1);
		metaMap.tileConfigs.put("house1", TileConfig{4});
		metaMap.tileConfigs.put("street_h_1", TileConfig{10});
		metaMap.tileConfigs.put("street_v_1", TileConfig{10});
		metaMap.tileConfigs.put("street_cross_1", TileConfig{1});
		metaMap.fixedTiles.push_back(FixedTile{"street_cross_1", 1, 1});
		return metaMap;
	}
};

TEST_F(MapBuilderTest, testConvertTileIdToMask) {
	EXPECT_TRUE(street_cross_1.convertTileIdToMask(0) == RMA_EVERYTHING_FITS);
	EXPECT_TRUE(street_cross_1.convertTileIdToMask(2) == RMA_EVERYTHING_FITS);
	EXPECT_TRUE(street_cross_1.convertTileIdToMask(6) == RMA_EVERYTHING_FITS);
	EXPECT_TRUE(street_cross_1.convertTileIdToMask(8) == RMA_EVERYTHING_FITS);
	EXPECT_TRUE(street_cross_1.convertTileIdToMask(4) & RMA_SOLID);
}

TEST_F(MapBuilderTest, testStreetFitsLeftCross) {
	int streetRight, crossLeft;
	Tile::oppositeIndices(Direction::Left, crossLeft, streetRight);
	const uint64_t crossMask = street_cross_1.convertTileIdToMask(crossLeft);
	const uint64_t streetMask = street_h_1.convertTileIdToMask(streetRight);
	EXPECT_TRUE(crossMask & streetMask) << "crossMask: " << crossMask << ", streeMask: " << streetMask;
}

TEST_F(MapBuilderTest, testStreetDoesNotFitLeftCross) {
	int streetRight, crossLeft;
	Tile::oppositeIndices(Direction::Left, crossLeft, streetRight);
	const uint64_t crossMask = street_cross_1.convertTileIdToMask(crossLeft);
	const uint64_t streetMask = street_v_1.convertTileIdToMask(streetRight);
	EXPECT_FALSE(crossMask & streetMask) << "crossMask: " << crossMask << ", streeMask: " << streetMask;
}

TEST_F(MapBuilderTest, testBuildMap) {
	const MetaMap &metaMap = createMetaMap();
	const LevelVolumes &volumes = buildMap(&metaMap, _volumeCache);
	EXPECT_FALSE(empty(volumes));
	EXPECT_TRUE(save(volumes));
	cleanup(volumes);
}

} // namespace rma
