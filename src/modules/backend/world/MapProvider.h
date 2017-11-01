/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "MapId.h"
#include <memory>
#include <unordered_map>

namespace backend {

class MapProvider {
public:
	using Maps = std::unordered_map<MapId, MapPtr>;
private:
	io::FilesystemPtr _filesystem;
	core::EventBusPtr _eventBus;
	std::unordered_map<MapId, MapPtr> _maps;
public:
	MapProvider(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus);

	MapPtr map(MapId id) const;

	Maps worldMaps() const;

	bool init();
	void shutdown();
};

typedef std::shared_ptr<MapProvider> MapProviderPtr;

}
