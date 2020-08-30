/**
 * @file
 */

#include "MapProvider.h"
#include "Map.h"
#include "io/Filesystem.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Assert.h"
#include "backend/entity/ai/AILoader.h"
#include "http/HttpServer.h"
#include "http/HttpMimeType.h"
#include "attrib/ContainerProvider.h"
#include "voxel/PagedVolume.h"
#include "voxelworld/WorldMgr.h"
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
		const bool empty = i == _maps.end();
		core_assert(!empty);
		if (!empty) {
			Log::warn("Could not find map for id %i", (int)id);
			return i->second;
		}
		Log::error("Could not find any valid map");
	} else {
		Log::debug("Could not find map for id %i", (int)id);
	}
	return MapPtr();
}

MapProvider::Maps MapProvider::worldMaps() const {
	return _maps;
}

bool MapProvider::init() {
	const core::String& lua = _filesystem->load("behaviourtrees.lua");
	if (!_loader->init(lua)) {
		Log::error("could not load the behaviourtrees: %s", _loader->getError().c_str());
		return false;
	}

	_httpServer->registerRoute(http::HttpMethod::GET, "/chunk", [&] (const http::RequestParser& request, http::HttpResponse* response) {
		core_trace_scoped(ChunkDownload);
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
		voxelworld::WorldMgr* worldMgr = m->worldMgr();
		voxel::PagedVolume* volume = worldMgr->volumeData();
		const glm::ivec3& chunkPos = volume->chunkPos(x, y, z);
		const core::VarPtr& seed = core::Var::getSafe(cfg::ServerSeed);
		persistence::Blob blob = persister->load(chunkPos.x, chunkPos.y, chunkPos.z, mapid, seed->uintVal());
		if (blob.length <= 0) {
			(void)volume->voxel(x, y, z);
			blob = persister->load(chunkPos.x, chunkPos.y, chunkPos.z, mapid, seed->uintVal());
			if (blob.length <= 0) {
				response->status = http::HttpStatus::NotFound;
				response->setText(core::string::format("Chunk not found at %i:%i:%i on map %i with seed %u",
						chunkPos.x, chunkPos.y, chunkPos.z, mapid, seed->uintVal()));
				return;
			}
		}
		response->body = (char*)core_malloc(blob.length);
		core_memcpy((void*)response->body, blob.data, blob.length);
		response->freeBody = true;
		response->contentLength(blob.length);
		response->headers.put(http::header::CONTENT_TYPE, http::mimetype::APPLICATION_CHUNK);
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
	Log::info("Map provider initialized with %i maps", (int)_maps.size());
	return true;
}

void MapProvider::shutdown() {
	_httpServer->unregisterRoute(http::HttpMethod::GET, "/chunk");
	for (auto& map : _maps) {
		map.second->shutdown();
	}
	_maps.clear();
}

}
