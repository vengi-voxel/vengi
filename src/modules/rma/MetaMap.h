/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "voxel/RawVolume.h"
#include "commonlua/LUA.h"

namespace lua {
class LUA;
}

namespace rma {

#define RMA_MAX_MAP_LEVEL 4
using LevelVolumes = core::Array<voxel::RawVolume *, RMA_MAX_MAP_LEVEL>;

// x and z dimensions
#define RMA_MAP_TILE_VOXEL_SIZE 64
// y dimension
#define RMA_MAP_LEVEL_VOXEL_HEIGHT 19

#define RMA_SOLID (1UL)
#define RMA_IS_PLACED(x) ((x)&RMA_SOLID)
#define RMA_EVERYTHING_FITS (~RMA_SOLID)

enum class Direction { Left, Up, Right, Down };

struct Tile {
	// this is a map of surrounding definitions of other tiles
	// and the center that holds the own tile definition
	core::String tiles3x3[9];

	uint64_t convertTileIdToMask(int idx) const;

	inline uint64_t ownMask() const {
		return convertTileIdToMask(4);
	}

	inline uint64_t leftMask() const {
		return convertTileIdToMask(3);
	}
	inline uint64_t rightMask() const {
		return convertTileIdToMask(5);
	}

	inline uint64_t upMask() const {
		return convertTileIdToMask(1);
	}
	inline uint64_t downMask() const {
		return convertTileIdToMask(7);
	}

	/**
	 * Direction indices into the 3x3 matrix
	 *
	 * @code
	 * 0 1 2
	 * 3 4 5
	 * 6 7 8
	 * @endcode
	 *
	 * @param[in] dir @c Directionto walk into
	 * @param[out] side1 The side of your own tile into the given direction
	 * @param[out] side2 The side of the neighbour tile for the given direction
	 */
	static void oppositeIndices(Direction dir, int &side1, int &side2);
};

struct TileConfig {
	int maximum;
};

struct FixedTile {
	core::String tileName;
	int x;
	int z;
};

/**
 * @brief This is a description of a liveless map object. The tower positions in the final map are - next to other
 * things - defined here.
 */
class MetaMap {
private:
	const core::String _name; //!< filename without extension

protected:
	virtual core::DynamicArray<luaL_Reg> luaExtensions() {
		return core::DynamicArray<luaL_Reg>{};
	}
public:
	MetaMap(const core::String &name);
	virtual ~MetaMap() {}

	bool load(const core::String &luaString);
	const core::String &name() const;

	core::String model; //!< model inside the maps folder - extension is optional
	core::String title;
	core::String image; //!< image inside the maps folder - extension is optional
	core::String description;

	// generator stuff
	int width = 3;
	int height = 3;
	core::StringMap<Tile> tiles;
	core::StringMap<TileConfig> tileConfigs;
	core::DynamicArray<FixedTile> fixedTiles;
};

inline const core::String &MetaMap::name() const {
	return _name;
}

typedef core::SharedPtr<MetaMap> MetaMapPtr;

} // namespace rma
