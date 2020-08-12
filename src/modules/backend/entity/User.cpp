/**
 * @file
 */

#include "User.h"
#include "attrib/Attributes.h"
#include "backend/world/Map.h"
#include "voxel/PagedVolume.h"
#include "voxelworld/WorldMgr.h"

namespace backend {

User::User(ENetPeer* peer, EntityId id,
		const core::String& name,
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
	core::DynamicArray<core::VarPtr> vars;
	core::Var::visitReplicate([&vars] (const core::VarPtr& var) {
		vars.push_back(var);
	});
	flatbuffers::FlatBufferBuilder fbb;
	auto fbbVars = fbb.CreateVector<flatbuffers::Offset<network::Var>>(vars.size(),
		[&] (size_t i) {
			const core::String& sname = vars[i]->name();
			const core::String& svalue = vars[i]->strVal();
			auto name = fbb.CreateString(sname.c_str(), sname.size());
			auto value = fbb.CreateString(svalue.c_str(), svalue.size());
			return network::CreateVar(fbb, name, value);
		});
	if (!_messageSender->sendServerMessage(_peer, fbb, network::ServerMsgType::VarUpdate,
			network::CreateVarUpdate(fbb, fbbVars).Union())) {
		Log::warn("Failed to send var message to the client");
	}
}

void User::shutdown() {
	Log::info("Shutdown user");
	_stockMgr.shutdown();
	_cooldownMgr.shutdown();
	_attribMgr.shutdown();
	_logoutMgr.shutdown();
	_movementMgr.shutdown();
	Super::shutdown();
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
	Log::info("connect user");
	_attribs.markAsDirty();
	sendVars();
	broadcastUserSpawn();
	broadcastUserinfo();
}

void User::onReconnect() {
	Log::info("reconnect user");
	visitVisible([&] (const EntityPtr& e) {
		sendEntitySpawn(e);
	});
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

void User::userinfo(const char *key, const char* value) {
	_userinfo.put(key, value);
}

void User::broadcastUserinfo() {
	flatbuffers::FlatBufferBuilder fbb;
	auto iter = _userinfo.begin();
	auto fbbVars = fbb.CreateVector<flatbuffers::Offset<network::Var>>(_userinfo.size(),
		[&] (size_t i, auto* iter) {
			auto name = fbb.CreateString((*iter)->key.c_str(), (*iter)->key.size());
			auto value = fbb.CreateString((*iter)->value.c_str(), (*iter)->value.size());
			++(*iter);
			return network::CreateVar(fbb, name, value);
		}, &iter);
	sendToVisible(fbb, network::ServerMsgType::UserInfo, network::CreateUserInfo(fbb, id(), fbbVars).Union(), true);
}

void User::broadcastUserSpawn() const {
	flatbuffers::FlatBufferBuilder fbb;
	const network::Vec3 pos { _pos.x, _pos.y, _pos.z };
	sendToVisible(fbb, network::ServerMsgType::UserSpawn, network::CreateUserSpawn(fbb, id(), fbb.CreateString(_name.c_str(), _name.size()), &pos).Union(), true);
}

bool User::sendMessage(flatbuffers::FlatBufferBuilder& fbb, network::ServerMsgType type, flatbuffers::Offset<void> msg) const {
	if (_peer == nullptr) {
		return false;
	}
	_messageSender->sendServerMessage(_peer, fbb, type, msg);
	return true;
}


}
