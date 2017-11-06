/**
 * @file
 */

#include "MapProvider.h"
#include "Map.h"
#include "io/Filesystem.h"
#include "core/Log.h"

namespace backend {

MapProvider::MapProvider(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus) :
		_filesystem(filesystem), _eventBus(eventBus) {
}

MapPtr MapProvider::map(MapId id, bool forceValidMap) const {
	auto i = _maps.find(id);
	if (i != _maps.end()) {
		return i->second;
	}
	if (forceValidMap) {
		i = _maps.begin();
		const bool empty = i != _maps.end();
		core_assert(!empty);
		if (!empty) {
			return i->second;
		}
	}
	return MapPtr();
}

MapProvider::Maps MapProvider::worldMaps() const {
	return _maps;
}

bool MapProvider::init() {
	const MapPtr& map = std::make_shared<Map>(1, _eventBus);
	if (!map->init(_filesystem)) {
		Log::warn("Failed to init map %i", map->id());
		return false;
	}
	_maps.insert(std::make_pair(1, map));
	return true;
}

void MapProvider::shutdown() {
	_maps.clear();
}

}
