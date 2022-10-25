/**
 * @file
 */

#include "MetaMap.h"
#include "LUAMetaMap.h"

namespace rma {

uint64_t Tile::convertTileIdToMask(int idx) const {
	uint64_t mask = 0l;
	for (char chr : tiles3x3[idx]) {
		if (chr == '+') {
			mask |= RMA_SOLID;
		} else if (chr == '0') {
			mask |= RMA_EVERYTHING_FITS;
		} else if (chr >= 'a' && chr <= 'z') {
			mask |= (uint64_t)1 << ((chr - 'a') + 1);
		}
	}
	return mask;
}

void Tile::oppositeIndices(Direction dir, int &side1, int &side2) {
	switch (dir) {
	case Direction::Left:
		side1 = 3;
		side2 = 5;
		break;
	case Direction::Right:
		side1 = 5;
		side2 = 3;
		break;
	case Direction::Up:
		side1 = 1;
		side2 = 7;
		break;
	case Direction::Down:
		side1 = 7;
		side2 = 1;
		break;
	}
}

MetaMap::MetaMap(const core::String &name) : _name(name), model(name) {
}

bool MetaMap::load(const core::String &luaString) {
	lua::LUA lua;
	luametamap_setup(lua, luaExtensions());
	if (!lua.load(luaString)) {
		Log::error("Failed to load lua script for map %s: %s", _name.c_str(), lua.error().c_str());
		return false;
	}

	lua_getglobal(lua, "init");
	if (lua_isnil(lua, -1)) {
		Log::error("Function init(map) wasn't found");
		return false;
	}

	luametamap_pushmetamap(lua, this);
	const int ret = lua_pcall(lua, 1, 0, 0);
	if (ret != LUA_OK) {
		Log::error("%s", lua_tostring(lua, -1));
		return false;
	}
	Log::debug("map %s loaded", _name.c_str());
	return true;
}

} // namespace rma
