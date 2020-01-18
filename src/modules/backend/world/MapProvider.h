/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "MapId.h"
#include "core/IComponent.h"
#include "http/HttpServer.h"
#include <memory>
#include <unordered_map>

namespace backend {

/**
 * @brief Responsible for loading and initializing the maps.
 */
class MapProvider : public core::IComponent {
public:
	using Maps = std::unordered_map<MapId, MapPtr>;
private:
	io::FilesystemPtr _filesystem;
	core::EventBusPtr _eventBus;
	core::TimeProviderPtr _timeProvider;
	EntityStoragePtr _entityStorage;
	network::ServerMessageSenderPtr _messageSender;
	AILoaderPtr _loader;
	attrib::ContainerProviderPtr _containerProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	persistence::PersistenceMgrPtr _persistenceMgr;
	voxelformat::VolumeCachePtr _volumeCache;
	http::HttpServerPtr _httpServer;

	std::unordered_map<MapId, MapPtr> _maps;
public:
	MapProvider(
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
			const http::HttpServerPtr& httpServer);
	~MapProvider();

	/**
	 * @brief Returns a valid and initialized map (if found)
	 * @param[in] id The @c MapId to get the map for
	 * @param[in] forceValidMap If this is set to @c true, you will get a default map in the case that
	 * no map could have been found for the given @c MapId
	 * @return If no map is found, an empty @c MapPtr object is returned. In some cases you might want
	 * to have a valid map in every case (e.g. login) - use the parameter @c forceValidMap in that case.
	 */
	MapPtr map(MapId id, bool forceValidMap = false) const;

	Maps worldMaps() const;

	bool init() override;
	void shutdown() override;
};

typedef std::shared_ptr<MapProvider> MapProviderPtr;

}
