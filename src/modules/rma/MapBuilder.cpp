/**
 * @file
 */

#include "MapBuilder.h"
#include "MetaMap.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/Array2DView.h"
#include "core/collection/DynamicArray.h"
#include "math/Random.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VoxelUtil.h"

namespace rma {

struct PlacedTile {
	core::String tileName;
	Tile tile;
};

using MapTileArray = core::DynamicArray<PlacedTile>;
using MapTileView = core::Array2DView<PlacedTile>;
using MapTileCountArray = core::StringMap<int>;

static bool checkTile(const MetaMap *metaMap, const MapTileView &view, int x, int z, const Tile &tile,
					  Direction direction) {
	int nx; // coordinate for the already placed tile
	int nz; // coordinate for the already placed tile
	switch (direction) {
	case Direction::Left:
		nx = x - 1;
		nz = z;
		break;
	case Direction::Up:
		nx = x;
		nz = z - 1;
		break;
	case Direction::Right:
		nx = x + 1;
		nz = z;
		break;
	case Direction::Down:
		nx = x;
		nz = z + 1;
		break;
	}
	// border values don't have constraints outside the map
	if (nx < 0 || nx >= metaMap->width || nz < 0 || nz >= metaMap->height) {
		return true;
	}
	const PlacedTile &pt = view.get(nx, nz);
	if (pt.tileName.empty()) {
		return true;
	}

	int placedIdx;
	int idx;
	Tile::oppositeIndices(direction, idx, placedIdx);

	const uint64_t mask = tile.convertTileIdToMask(idx);
	const uint64_t placedMask = pt.tile.convertTileIdToMask(placedIdx);
	return mask & placedMask;
}

static bool checkTileFits(const MetaMap *metaMap, const MapTileView &view, int x, int z, const Tile &tile) {
	if (!checkTile(metaMap, view, x, z, tile, Direction::Left)) {
		return false;
	}
	if (!checkTile(metaMap, view, x, z, tile, Direction::Up)) {
		return false;
	}
	// this is just for fixed placed tiles - otherwise we would just look backwards
	if (!checkTile(metaMap, view, x, z, tile, Direction::Right)) {
		return false;
	}
	if (!checkTile(metaMap, view, x, z, tile, Direction::Down)) {
		return false;
	}
	return true;
}

static void fillFixedTiles(const MetaMap *&metaMap, MapTileCountArray &cnt, MapTileView &view) {
	Log::debug("Fill fixed tiles: %i", (int)metaMap->fixedTiles.size());
	for (const FixedTile &fixedTile : metaMap->fixedTiles) {
		const auto it = metaMap->tiles.find(fixedTile.tileName);
		if (it == metaMap->tiles.end()) {
			Log::error("Failed to find tile %s", fixedTile.tileName.c_str());
			continue;
		}
		const int x = fixedTile.x;
		const int z = fixedTile.z;
		if (!view.get(x, z).tileName.empty()) {
			Log::warn("Fixed tile can't get placed - the field is already occupied");
			continue;
		}
		const PlacedTile placedTile{fixedTile.tileName, it->value};
		view.set(x, z, placedTile);
		int count = 0;
		cnt.get(fixedTile.tileName, count);
		cnt.put(fixedTile.tileName, ++count);
		Log::debug("Place fixed tile %s at %i:%i", fixedTile.tileName.c_str(), x, z);
	}
}

static bool tryToPlaceTile(int x, int z, const core::String &tileId, const Tile &tile, const MetaMap *&metaMap,
						   MapTileCountArray &cnt, MapTileView &view) {
	int count = 0;
	cnt.get(tileId, count);

	const auto it = metaMap->tileConfigs.find(tileId);
	if (it != metaMap->tileConfigs.end()) {
		const TileConfig &cfg = it->value;
		if (count >= cfg.maximum) {
			Log::debug("max count for %s reached", tileId.c_str());
			return false;
		}
	}
	if (checkTileFits(metaMap, view, x, z, tile)) {
		const PlacedTile placedTile{it->key, tile};
		view.set(x, z, placedTile);
		cnt.put(tileId, ++count);
		Log::debug("tile fits %s at %i:%i", tileId.c_str(), x, z);
		return true;
	}
	Log::debug("tile doesn't fit %s at %i:%i", tileId.c_str(), x, z);
	return false;
}

static void fillSuitableTiles(math::Random &rnd, const MetaMap *&metaMap, MapTileCountArray &cnt, MapTileView &view) {
	const int w = metaMap->width;
	for (int i = 0; i < (int)view.size(); ++i) {
		const int x = i % w;
		const int z = i / w;
		if (!view.get(x, z).tileName.empty()) {
			continue;
		}
		for (const auto &e : metaMap->tiles) {
			const float r = rnd.randomf();
			if (r >= 0.8f) {
				continue;
			}
			if (tryToPlaceTile(x, z, e->key, e->value, metaMap, cnt, view)) {
				break;
			}
		}
	}
}

static void fillGaps(const MetaMap *&metaMap, MapTileCountArray &cnt, MapTileView &view) {
	const int w = metaMap->width;
	for (int i = 0; i < (int)view.size(); ++i) {
		const int x = i % w;
		const int z = i / w;
		if (!view.get(x, z).tileName.empty()) {
			continue;
		}
		for (const auto &e : metaMap->tiles) {
			if (tryToPlaceTile(x, z, e->key, e->value, metaMap, cnt, view)) {
				break;
			}
		}
	}
}

static bool isCompleted(const MetaMap *&metaMap, MapTileCountArray &cnt, MapTileView &view) {
	const int w = metaMap->width;
	for (int i = 0; i < (int)view.size(); ++i) {
		const int x = i % w;
		const int z = i / w;
		if (view.get(x, z).tileName.empty()) {
			return false;
		}
	}
	return true;
}

static core::String getPathForTileName(const core::String &tileName, int level) {
	return core::string::format("maps/%s_%i", tileName.c_str(), level);
}

static void createVolumes(LevelVolumes &volumes, const MapTileView &view,
						  const voxelformat::VolumeCachePtr &volumeCache) {
	const int w = view.width();
	const int h = view.height();
	for (int level = 0; level < (int)volumes.size(); ++level) {
		const int minsY = RMA_MAP_LEVEL_VOXEL_HEIGHT * level;
		const int maxsX = w * RMA_MAP_TILE_VOXEL_SIZE - 1;
		const int maxsZ = maxsX;
		const int maxsY = minsY + RMA_MAP_LEVEL_VOXEL_HEIGHT - 1;
		const voxel::Region region(0, minsY, 0, maxsX, maxsY, maxsZ);
		voxel::RawVolume *finalVolume = new voxel::RawVolume(region);
		for (int x = 0; x < w; ++x) {
			for (int z = 0; z < h; ++z) {
				const PlacedTile &placedTile = view.get(x, z);
				if (placedTile.tileName.empty()) {
					Log::warn("Failed to place a tile at %i:%i", x, z);
					continue;
				}
				const core::String &tilePath = getPathForTileName(placedTile.tileName, level);
				const voxel::RawVolume *v = volumeCache->loadVolume(tilePath);
				if (v == nullptr) {
					if (level == 0) {
						Log::warn("Could not find map tile for %s", tilePath.c_str());
					}
					continue;
				}
				const int tminsx = x * RMA_MAP_TILE_VOXEL_SIZE;
				const int tminsy = level * RMA_MAP_LEVEL_VOXEL_HEIGHT;
				const int tminsz = z * RMA_MAP_TILE_VOXEL_SIZE;
				const int tmaxsx = tminsx + RMA_MAP_TILE_VOXEL_SIZE;
				const int tmaxsy = tminsy + RMA_MAP_LEVEL_VOXEL_HEIGHT;
				const int tmaxsz = tminsz + RMA_MAP_TILE_VOXEL_SIZE;
				const voxel::Region targetRegion(tminsx, tminsy, tminsz, tmaxsx, tmaxsy, tmaxsz);
				voxelutil::copyIntoRegion(*v, *finalVolume, targetRegion);
			}
		}
		volumes[level] = finalVolume;
	}
}

LevelVolumes buildMap(const MetaMap *metaMap, const voxelformat::VolumeCachePtr &volumeCache, unsigned int seed) {
	core_assert(!metaMap->tiles.empty());
	const int w = metaMap->width;
	const int h = metaMap->height;

	LevelVolumes levels;
	levels.fill(nullptr);

	math::Random rnd(seed);

	// assemble the map
	const int maxRuns = 4;
	for (int i = 0; i < maxRuns; ++i) {
		MapTileArray map(w * h);
		MapTileView view(map.data(), w, h);
		MapTileCountArray cnt;
		fillFixedTiles(metaMap, cnt, view);
		fillSuitableTiles(rnd, metaMap, cnt, view);
		fillGaps(metaMap, cnt, view);
		if (isCompleted(metaMap, cnt, view)) {
			for (int i = 0; i < (int)view.size(); ++i) {
				const int x = i % w;
				const int y = i / w;
				const PlacedTile &placedTile = view.get(x, y);
				Log::debug("%i:%i => %s", x, y, placedTile.tileName.c_str());
			}

			// create the map levels
			createVolumes(levels, view, volumeCache);
			break;
		}
		Log::warn("Failed to assemble map with run %i/%i", i, maxRuns);
	}

	// TODO: improve this and replace the dummy algo
	// https://ijdykeman.github.io/ml/2017/10/12/wang-tile-procedural-generation.html
	// https://en.wikipedia.org/wiki/Wang_tile
	// https://nothings.org/gamedev/herringbone/herringbone_src.html
	return levels;
}

} // namespace rma
