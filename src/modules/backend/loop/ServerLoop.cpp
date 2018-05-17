/**
 * @file
 */

#include "ClientMessages_generated.h"
#include "ServerLoop.h"

#include "core/command/Command.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "cooldown/CooldownProvider.h"
#include "attrib/ContainerProvider.h"
#include "persistence/ConnectionPool.h"
#include "BackendModels.h"
#include "EventMgrModels.h"
#include "backend/metric/MetricMgr.h"
#include "backend/entity/User.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/AICommon.h"
#include "backend/network/UserConnectHandler.h"
#include "backend/network/UserConnectedHandler.h"
#include "backend/network/UserDisconnectHandler.h"
#include "backend/network/AttackHandler.h"
#include "backend/network/MoveHandler.h"
#include "persistence/PersistenceMgr.h"
#include "backend/world/World.h"
#include "core/command/CommandHandler.h"
#include "voxel/MaterialColor.h"
#include "eventmgr/EventMgr.h"
#include "stock/StockDataProvider.h"

namespace backend {

ServerLoop::ServerLoop(const core::TimeProviderPtr& timeProvider, const MapProviderPtr& mapProvider,
		const network::ServerMessageSenderPtr& messageSender,
		const WorldPtr& world, const persistence::DBHandlerPtr& dbHandler,
		const network::ServerNetworkPtr& network, const io::FilesystemPtr& filesystem,
		const EntityStoragePtr& entityStorage, const core::EventBusPtr& eventBus,
		const attrib::ContainerProviderPtr& containerProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider, const eventmgr::EventMgrPtr& eventMgr,
		const stock::StockDataProviderPtr& stockDataProvider, const MetricMgrPtr& metricMgr,
		const persistence::PersistenceMgrPtr& persistenceMgr) :
		_network(network), _timeProvider(timeProvider), _mapProvider(mapProvider), _messageSender(messageSender),
		_world(world),
		_entityStorage(entityStorage), _eventBus(eventBus), _attribContainerProvider(containerProvider),
		_cooldownProvider(cooldownProvider), _eventMgr(eventMgr), _dbHandler(dbHandler),
		_stockDataProvider(stockDataProvider), _metricMgr(metricMgr), _filesystem(filesystem),
		_persistenceMgr(persistenceMgr) {
	_eventBus->subscribe<network::DisconnectEvent>(*this);
}

bool ServerLoop::addTimer(uv_timer_t* timer, uv_timer_cb cb, uint64_t repeatMillis, uint64_t initialDelayMillis) {
	timer->data = this;
	uv_timer_init(_loop, timer);
	return uv_timer_start(timer, cb, initialDelayMillis, repeatMillis) == 0;
}

void ServerLoop::signalCallback(uv_signal_t* handle, int signum) {
	if (signum == SIGHUP) {
		core::App::getInstance()->requestQuit();
		return;
	}

	if (signum == SIGINT) {
		//ServerLoop* loop = (ServerLoop*)handle->data;
		// TODO: only quit if this was hit twice in under 2 seconds
		core::App::getInstance()->requestQuit();
		return;
	}
}

void ServerLoop::onIdle(uv_idle_t* handle) {
	ServerLoop* loop = (ServerLoop*)handle->data;
	const metric::MetricPtr& metric = loop->_metricMgr->metric();

	const core::App* app = core::App::getInstance();
	const int deltaFrame = app->deltaFrame();
	constexpr int delta = 10;
	if (std::abs(deltaFrame - loop->_lastDeltaFrame) > delta) {
		metric->timing("frame.delta", app->deltaFrame());
		loop->_lastDeltaFrame = deltaFrame;
	}
	const uint64_t lifetimeSeconds = app->lifetimeInSeconds();
	if (lifetimeSeconds != loop->_lifetimeSeconds) {
		metric->gauge("uptime", lifetimeSeconds);
		loop->_lifetimeSeconds = lifetimeSeconds;
	}
}

void ServerLoop::construct() {
	core::Command::registerCommand("sv_killnpc", [this] (const core::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: sv_killnpc <entityid>");
			return;
		}
		const EntityId id = core::string::toLong(args[0]);
		const NpcPtr& npc = _entityStorage->npc(id);
		if (!npc) {
			Log::info("No npc with id " PRIEntId " found", id);
			return;
		}
		if (!npc->die()) {
			Log::info("Could not kill npc with id " PRIEntId, id);
		} else {
			Log::info("Killed npc with id " PRIEntId, id);
		}
	}).setHelp("Kill npc with given entity id");

	_world->construct();
}

#define regHandler(type, handler, ...) \
	r->registerHandler(network::EnumNameClientMsgType(type), std::make_shared<handler>(__VA_ARGS__));

bool ServerLoop::init() {
	_loop = new uv_loop_t;
	if (uv_loop_init(_loop) != 0) {
		Log::error("Failed to init event loop");
		return false;
	}
	if (!_metricMgr->init()) {
		Log::warn("Failed to init metric sender");
	}
	if (!_dbHandler->init()) {
		Log::error("Failed to init the dbhandler");
		return false;
	}
	if (!_dbHandler->createTable(db::UserModel())) {
		Log::error("Failed to create user table");
		return false;
	}
	if (!_dbHandler->createTable(db::AttribModel())) {
		Log::error("Failed to create attrib table");
		return false;
	}
	if (!_dbHandler->createTable(db::InventoryModel())) {
		Log::error("Failed to create stock table");
		return false;
	}
	if (!_dbHandler->createTable(db::CooldownModel())) {
		Log::error("Failed to create cooldown table");
		return false;
	}

	const std::string& events = _filesystem->load("events.lua");
	if (!_eventMgr->init(events)) {
		Log::error("Failed to init event manager");
		return false;
	}

	const std::string& cooldowns = _filesystem->load("cooldowns.lua");
	if (!_cooldownProvider->init(cooldowns)) {
		Log::error("Failed to load the cooldown configuration: %s", _cooldownProvider->error().c_str());
		return false;
	}

	const std::string& stockLuaString = _filesystem->load("stock.lua");
	if (!_stockDataProvider->init(stockLuaString)) {
		Log::error("Failed to load the stock configuration: %s", _stockDataProvider->error().c_str());
		return false;
	}

	const std::string& attributes = _filesystem->load("attributes.lua");
	if (!_attribContainerProvider->init(attributes)) {
		Log::error("Failed to load the attributes: %s", _attribContainerProvider->error().c_str());
		return false;
	}

	if (!_persistenceMgr->init()) {
		Log::error("Failed to init the persistence manager");
		return false;
	}

	const network::ProtocolHandlerRegistryPtr& r = _network->registry();
	regHandler(network::ClientMsgType::UserConnect, UserConnectHandler,
			_network, _mapProvider, _dbHandler, _persistenceMgr, _entityStorage, _messageSender,
			_timeProvider, _attribContainerProvider, _cooldownProvider, _stockDataProvider);
	regHandler(network::ClientMsgType::UserConnected, UserConnectedHandler);
	regHandler(network::ClientMsgType::UserDisconnect, UserDisconnectHandler);
	regHandler(network::ClientMsgType::Attack, AttackHandler);
	regHandler(network::ClientMsgType::Move, MoveHandler);

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return false;
	}

	if (!_world->init()) {
		Log::error("Failed to init the world");
		return false;
	}

	addTimer(&_worldTimer, [] (uv_timer_t* handle) {
		const ServerLoop* loop = (const ServerLoop*)handle->data;
		loop->_world->update(handle->repeat);
	}, 1000);

	addTimer(&_persistenceMgrTimer, [] (uv_timer_t* handle) {
		const ServerLoop* loop = (const ServerLoop*)handle->data;
		const long dt = handle->repeat;
		const persistence::PersistenceMgrPtr& persistenceMgr = loop->_persistenceMgr;
		core::App::getInstance()->threadPool().enqueue([=] () {
			persistenceMgr->update(dt);
		});
	}, 10000);

	_idleTimer.data = this;
	if (uv_idle_init(_loop, &_idleTimer) != 0) {
		Log::warn("Couldn't init the idle timer");
		return false;
	}
	uv_idle_start(&_idleTimer, onIdle);

	uv_signal_init(_loop, &_signal);
	_signal.data = this;
	uv_signal_start(&_signal, signalCallback, SIGHUP);
	uv_signal_start(&_signal, signalCallback, SIGINT);

	if (!_input.init(_loop)) {
		Log::warn("Could not init console input");
	}

	// init the network last...
	if (!_network->init()) {
		Log::error("Failed to init the network");
		return false;
	}

	const core::VarPtr& port = core::Var::getSafe(cfg::ServerPort);
	const core::VarPtr& host = core::Var::getSafe(cfg::ServerHost);
	const core::VarPtr& maxclients = core::Var::getSafe(cfg::ServerMaxClients);
	if (!_network->bind(port->intVal(), host->strVal(), maxclients->intVal(), 2)) {
		Log::error("Failed to bind the server socket on %s:%i", host->strVal().c_str(), port->intVal());
		return false;
	}
	Log::info("Server socket is up at %s:%i", host->strVal().c_str(), port->intVal());

	return true;
}

void ServerLoop::shutdown() {
	uv_signal_stop(&_signal);
	_persistenceMgr->shutdown();
	_world->shutdown();
	_dbHandler->shutdown();
	_metricMgr->shutdown();
	_input.shutdown();
	_network->shutdown();
	uv_timer_stop(&_worldTimer);
	uv_timer_stop(&_persistenceMgrTimer);
	uv_idle_stop(&_idleTimer);
	uv_tty_reset_mode();
	if (_loop != nullptr) {
		uv_loop_close(_loop);
		delete _loop;
		_loop = nullptr;
	}
}

void ServerLoop::update(long dt) {
	core_trace_scoped(ServerLoop);
	// not everything is ticked in here directly, a lot is handled by libuv timers
	uv_run(_loop, UV_RUN_NOWAIT);
	_network->update();
	const int eventSkip = _eventBus->update(200);
	if (eventSkip != _lastEventSkip) {
		_metricMgr->metric()->gauge("events.skip", eventSkip);
		_lastEventSkip = eventSkip;
	}
}

// TODO: doesn't belong here
void ServerLoop::onEvent(const network::DisconnectEvent& event) {
	ENetPeer* peer = event.peer();
	Log::info("disconnect peer: %u", peer->connectID);
	User* user = reinterpret_cast<User*>(peer->data);
	if (user == nullptr) {
		return;
	}
	user->logoutMgr().triggerLogout();
}

}
