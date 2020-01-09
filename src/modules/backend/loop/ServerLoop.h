/**
 * @file
 */

#pragma once

#include "core/EventBus.h"
#include "core/Trace.h"
#include "core/Input.h"
#include "core/EventBus.h"
#include "core/IComponent.h"
#include "network/ServerNetwork.h"
#include "network/NetworkEvents.h"
#include "backend/ForwardDecl.h"
#include "backend/world/World.h"
#include "network/ProtocolHandlerRegistry.h"
#include "backend/entity/EntityStorage.h"
#include "persistence/DBHandler.h"

#include <uv.h>

namespace backend {

class ServerLoop:
	public core::IComponent,
	public core::IEventBusHandler<network::DisconnectEvent> {
private:
	network::ServerNetworkPtr _network;
	core::TimeProviderPtr _timeProvider;
	MapProviderPtr _mapProvider;
	network::ServerMessageSenderPtr _messageSender;
	WorldPtr _world;
	EntityStoragePtr _entityStorage;
	core::EventBusPtr _eventBus;
	attrib::ContainerProviderPtr _attribContainerProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	eventmgr::EventMgrPtr _eventMgr;
	persistence::DBHandlerPtr _dbHandler;
	stock::StockDataProviderPtr _stockDataProvider;
	core::Input _input;
	MetricMgrPtr _metricMgr;
	io::FilesystemPtr _filesystem;
	persistence::PersistenceMgrPtr _persistenceMgr;
	voxelformat::VolumeCachePtr _volumeCache;

	uv_loop_t *_loop = nullptr;
	uv_timer_t _worldTimer;
	uv_timer_t _persistenceMgrTimer;
	uv_idle_t _idleTimer;
	uv_signal_t _signal;

	int _lastEventSkip = 0;
	int _lastDeltaFrame = 0;
	uint64_t _lifetimeSeconds = 0u;

	static void onIdle(uv_idle_t* handle);
	static void signalCallback(uv_signal_t* handle, int signum);
	bool addTimer(uv_timer_t* timer, uv_timer_cb cb, uint64_t repeatMillis, uint64_t initialDelayMillis = 0);
public:
	ServerLoop(const core::TimeProviderPtr& timeProvider, const MapProviderPtr& mapProvider,
			const network::ServerMessageSenderPtr& messageSender,
			const WorldPtr& world, const persistence::DBHandlerPtr& dbHandler,
			const network::ServerNetworkPtr& network, const io::FilesystemPtr& filesystem,
			const EntityStoragePtr& entityStorage, const core::EventBusPtr& eventBus,
			const attrib::ContainerProviderPtr& containerProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider,
			const eventmgr::EventMgrPtr& eventMgr, const stock::StockDataProviderPtr& stockDataProvider,
			const MetricMgrPtr& metricMgr, const persistence::PersistenceMgrPtr& persistenceMgr,
			const voxelformat::VolumeCachePtr& volumeCache);

	void construct() override;
	bool init() override;
	void shutdown() override;
	void update(long dt);
	void onEvent(const network::DisconnectEvent& event) override;
};

typedef std::shared_ptr<ServerLoop> ServerLoopPtr;

}
