/**
 * @file
 */

#include "MapProvider.h"
#include "Map.h"
#include "core/io/Filesystem.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "backend/entity/ai/AILoader.h"

namespace backend {

MapProvider::MapProvider(
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider,
		const EntityStoragePtr& entityStorage,
		const network::ServerMessageSenderPtr& messageSender,
		const AILoaderPtr& loader,
		const attrib::ContainerProviderPtr& containerProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider,
		const persistence::PersistenceMgrPtr& persistenceMgr,
		const voxelformat::VolumeCachePtr& volumeCache) :
		_filesystem(filesystem), _eventBus(eventBus), _timeProvider(timeProvider),
		_entityStorage(entityStorage), _messageSender(messageSender), _loader(loader),
		_containerProvider(containerProvider), _cooldownProvider(cooldownProvider),
		_persistenceMgr(persistenceMgr), _volumeCache(volumeCache) {
}

MapProvider::~MapProvider() {
	shutdown();
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
	const std::string& lua = _filesystem->load("behaviourtrees.lua");
	if (!_loader->init(lua)) {
		Log::error("could not load the behaviourtrees: %s", _loader->getError().c_str());
		return false;
	}

	const MapPtr& map = std::make_shared<Map>(1, _eventBus, _timeProvider,
			_filesystem, _entityStorage, _messageSender, _volumeCache,
			_loader, _containerProvider, _cooldownProvider, _persistenceMgr);
	if (!map->init()) {
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
