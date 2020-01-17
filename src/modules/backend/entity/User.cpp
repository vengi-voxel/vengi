/**
 * @file
 */

#include "User.h"
#include "attrib/Attributes.h"
#include "backend/world/Map.h"
#include "network/ServerMessageSender.h"
#include "voxel/PagedVolume.h"
#include "voxelworld/WorldMgr.h"

namespace backend {

User::User(ENetPeer* peer, EntityId id,
		const std::string& name,
		const MapPtr& map,
		const network::ServerMessageSenderPtr& messageSender,
		const core::TimeProviderPtr& timeProvider,
		const attrib::ContainerProviderPtr& containerProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider,
		const persistence::DBHandlerPtr& dbHandler,
		const persistence::PersistenceMgrPtr& persistenceMgr,
		const stock::StockDataProviderPtr& stockDataProvider) :
		Super(id, map, messageSender, timeProvider, containerProvider),
		_name(name),
		_dbHandler(dbHandler),
		_timeProvider(timeProvider),
		_cooldownProvider(cooldownProvider),
		_stockMgr(this, stockDataProvider, dbHandler),
		_cooldownMgr(this, timeProvider, cooldownProvider, dbHandler, persistenceMgr),
		_attribMgr(id, _attribs, dbHandler, persistenceMgr),
		_logoutMgr(_cooldownMgr),
		_movementMgr(this) {
	setPeer(peer);
	_entityType = network::EntityType::PLAYER;
}

User::~User() {
}

void User::init() {
	Super::init();
	_stockMgr.init();
	_cooldownMgr.init();
	_attribMgr.init();
	_logoutMgr.init();
	_movementMgr.init();
}

void User::sendVars() const {
	std::vector<core::VarPtr> vars;
	core::Var::visitReplicate([&vars] (const core::VarPtr& var) {
		vars.push_back(var);
	});
	Log::info("send cvars to client: %i", (int)vars.size());
	flatbuffers::FlatBufferBuilder fbb;
	auto fbbVars = fbb.CreateVector<flatbuffers::Offset<network::Var>>(vars.size(),
		[&] (size_t i) {
			auto name = fbb.CreateString(vars[i]->name());
			auto value = fbb.CreateString(vars[i]->strVal());
			return network::CreateVar(fbb, name, value);
		});
	if (!_messageSender->sendServerMessage(_peer, fbb, network::ServerMsgType::VarUpdate,
			network::CreateVarUpdate(fbb, fbbVars).Union())) {
		Log::warn("Failed to send var message to the client");
	}
}

void User::shutdown() {
	Log::info("Shutdown user");
	Super::shutdown();
	_stockMgr.shutdown();
	_cooldownMgr.shutdown();
	_attribMgr.shutdown();
	_logoutMgr.shutdown();
	_movementMgr.shutdown();
}

ENetPeer* User::setPeer(ENetPeer* peer) {
	ENetPeer* old = _peer;
	_peer = peer;
	if (_peer) {
		_peer->data = this;
	}
	return old;
}

void User::onConnect() {
	Log::trace("connect user");
	sendVars();
	// TODO: remove init message? we replicate sv_seed now
	const long seed = core::Var::getSafe(cfg::ServerSeed)->longVal();
	sendInit(seed);
	sendUserSpawn();
	// TODO: send attributes to the client
}

void User::onReconnect() {
	Log::trace("reconnect user");
	sendVars();
	// TODO: remove init message? we replicate sv_seed now
	const long seed = core::Var::getSafe(cfg::ServerSeed)->longVal();
	sendInit(seed);
	visitVisible([&] (const EntityPtr& e) {
		sendEntitySpawn(e);
	});
	// TODO: send attributes to the client
}

bool User::update(long dt) {
	if (_logoutMgr.isDisconnect()) {
		return false;
	}
	if (!Super::update(dt)) {
		return false;
	}

	_stockMgr.update(dt);
	_cooldownMgr.update();
	_movementMgr.update(dt);
	_logoutMgr.update(dt);

	return true;
}

void User::sendInit(long seed) const {
	flatbuffers::FlatBufferBuilder fbb;
	sendMessage(fbb, network::ServerMsgType::Init, network::CreateInit(fbb, seed).Union());
}

void User::sendUserSpawn() const {
	flatbuffers::FlatBufferBuilder fbb;
	const network::Vec3 pos { _pos.x, _pos.y, _pos.z };
	sendToVisible(fbb, network::ServerMsgType::UserSpawn, network::CreateUserSpawn(fbb, id(), fbb.CreateString(_name), &pos).Union(), true);
}

bool User::sendMessage(flatbuffers::FlatBufferBuilder& fbb, network::ServerMsgType type, flatbuffers::Offset<void> msg) const {
	if (_peer == nullptr) {
		return false;
	}
	_messageSender->sendServerMessage(_peer, fbb, type, msg);
	return true;
}

}
