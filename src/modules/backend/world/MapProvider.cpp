/**
 * @file
 */

#include "MapProvider.h"
#include "Map.h"
#include "core/io/Filesystem.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "backend/entity/ai/AILoader.h"
#include "http/HttpServer.h"
#include "voxel/PagedVolume.h"
#include <glm/vec3.hpp>

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
		const voxelformat::VolumeCachePtr& volumeCache,
		const http::HttpServerPtr& httpServer,
		const core::Factory<DBChunkPersister>& chunkPersisterFactory,
		const persistence::DBHandlerPtr& dbHandler) :
		_filesystem(filesystem), _eventBus(eventBus), _timeProvider(timeProvider),
		_entityStorage(entityStorage), _messageSender(messageSender), _loader(loader),
		_containerProvider(containerProvider), _cooldownProvider(cooldownProvider),
		_persistenceMgr(persistenceMgr), _volumeCache(volumeCache), _httpServer(httpServer),
		_chunkPersisterFactory(chunkPersisterFactory), _dbHandler(dbHandler) {
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

	_httpServer->registerRoute(http::HttpMethod::GET, "/chunk", [&] (const http::RequestParser& request, http::HttpResponse* response) {
		HTTP_QUERY_GET_INT(x);
		HTTP_QUERY_GET_INT(y);
		HTTP_QUERY_GET_INT(z);
		HTTP_QUERY_GET_INT(mapid);
		const MapPtr& m = map(mapid);
		if (!m) {
			response->status = http::HttpStatus::NotFound;
			response->setText("Map with given id not found");
			return;
		}
		const DBChunkPersisterPtr& persister = m->chunkPersister();
		const core::VarPtr& seed = core::Var::getSafe(cfg::ServerSeed);
		persistence::Blob blob = persister->load(x, y, z, mapid, seed->uintVal());
		if (blob.length <= 0) {
			response->status = http::HttpStatus::NotFound;
			response->setText("Chunk not found");
			return;
		}
		response->body = (char*)core_malloc(blob.length);
		::memcpy((void*)response->body, blob.data, blob.length);
		response->freeBody = true;
		response->contentLength(blob.length);
		response->headers.put(http::header::CONTENT_TYPE, "application/chunk");
		blob.release();
	});

	const MapId mapId = 1;
	const MapPtr& map = std::make_shared<Map>(mapId, _eventBus, _timeProvider,
			_filesystem, _entityStorage, _messageSender, _volumeCache,
			_loader, _containerProvider, _cooldownProvider, _persistenceMgr,
			_chunkPersisterFactory.create(_dbHandler, mapId));
	if (!map->init()) {
		Log::warn("Failed to init map %i", mapId);
		return false;
	}
	_maps.insert(std::make_pair(mapId, map));
	return true;
}

void MapProvider::shutdown() {
	_httpServer->unregisterRoute(http::HttpMethod::GET, "/chunk");
	_maps.clear();
}

}
