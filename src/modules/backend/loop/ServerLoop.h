/**
 * @file
 */

#pragma once

#include "core/EventBus.h"
#include "core/Trace.h"
#include "metric/Metric.h"
#include "metric/MetricEvent.h"
#include "metric/IMetricSender.h"
#include "network/ServerNetwork.h"
#include "network/NetworkEvents.h"
#include "voxel/World.h"
#include "backend/ForwardDecl.h"
#include "backend/spawn/SpawnMgr.h"
#include "core/Input.h"
#include "network/ProtocolHandlerRegistry.h"
#include "backend/entity/EntityStorage.h"
#include "core/EventBus.h"
#include "persistence/DBHandler.h"
#include <uv.h>

#include <memory>
#include <thread>
#include <chrono>

namespace backend {

class ServerLoop:
	public core::IEventBusHandler<metric::MetricEvent>,
	public core::IEventBusHandler<network::NewConnectionEvent>,
	public core::IEventBusHandler<network::DisconnectEvent> {
private:
	network::ServerNetworkPtr _network;
	SpawnMgrPtr _spawnMgr;
	voxel::WorldPtr _world;
	ai::Zone* _zone = nullptr;
	ai::Server* _aiServer = nullptr;
	EntityStoragePtr _entityStorage;
	core::EventBusPtr _eventBus;
	AIRegistryPtr _registry;
	attrib::ContainerProviderPtr _attribContainerProvider;
	poi::PoiProviderPtr _poiProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	eventmgr::EventMgrPtr _eventMgr;
	persistence::DBHandlerPtr _dbHandler;
	stock::StockProviderPtr _stockDataProvider;
	core::Input _input;
	metric::Metric _metric;
	metric::IMetricSenderPtr _metricSender;

	uv_loop_t *_loop = nullptr;
	uv_timer_t _poiTimer;
	uv_timer_t _worldTimer;
	uv_timer_t _aiServerTimer;
	uv_timer_t _zoneTimer;
	uv_timer_t _spawnMgrTimer;
	uv_timer_t _entityStorageTimer;

	bool addTimer(uv_timer_t* timer, uv_timer_cb cb, uint64_t repeatMillis, uint64_t initialDelayMillis = 0);
public:
	ServerLoop(const persistence::DBHandlerPtr& dbHandler, const network::ServerNetworkPtr& network, const SpawnMgrPtr& spawnMgr,
			const voxel::WorldPtr& world, const EntityStoragePtr& entityStorage, const core::EventBusPtr& eventBus,
			const AIRegistryPtr& registry, const attrib::ContainerProviderPtr& containerProvider,
			const poi::PoiProviderPtr& poiProvider, const cooldown::CooldownProviderPtr& cooldownProvider,
			const eventmgr::EventMgrPtr& eventMgr, const stock::StockProviderPtr& stockDataProvider,
			const metric::IMetricSenderPtr& metricSender);

	bool init();
	void shutdown();
	void update(long dt);
	void onEvent(const network::DisconnectEvent& event);
	void onEvent(const network::NewConnectionEvent& event);
	void onEvent(const metric::MetricEvent& event);
};

typedef std::shared_ptr<ServerLoop> ServerLoopPtr;

}
