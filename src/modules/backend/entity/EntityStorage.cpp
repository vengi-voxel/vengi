#include "EntityStorage.h"
#include "core/Var.h"
#include "User.h"
#include "backend/storage/Persister.h"
#include "backend/storage/UserStore.h"

#define broadcastMsg(msg, type) _messageSender->broadcastServerMessage(fbb, network::messages::server::type, network::messages::server::msg.Union());

namespace backend {

EntityStorage::EntityStorage(network::MessageSenderPtr messageSender, voxel::WorldPtr world, core::TimeProviderPtr timeProvider,
		attrib::ContainerProviderPtr containerProvider, PoiProviderPtr poiProvider) :
		_quadTree(core::RectFloat::getMaxRect(), 100.0f), _quadTreeCache(_quadTree), _messageSender(messageSender), _world(world), _timeProvider(
				timeProvider), _containerProvider(containerProvider), _poiProvider(poiProvider), _time(0L) {
}

core::RectFloat EntityStorage::Node::getRect() const {
	return entity->rect();
}

bool EntityStorage::Node::operator==(const Node& rhs) const {
	return rhs.entity == entity;
}

void EntityStorage::registerUser(const UserPtr& user) {
	_users[user->id()] = user;
}

EntityId EntityStorage::getUserId(const std::string& user, const std::string& passwd) const {
	std::string tmUid = "0";
	Persister pq;
	pq.init();
	int checkId = pq.loadUser(user, passwd, tmUid);

	if (checkId == 0) {
		const core::VarPtr& autoReg = core::Var::get(cfg::ServerAutoRegister, "true");
		if (autoReg->boolVal()) {
			pq.storeUser(user, passwd, tmUid);
			checkId = pq.loadUser(user, passwd, tmUid);
		}
	}
	return checkId;
}

UserPtr EntityStorage::login(ENetPeer* peer, const std::string& email, const std::string& passwd) {
	EntityId id = getUserId(email, passwd);
	Log::info("getting userid: %i", (int) id);
	if (id <= 0)
		return UserPtr();
	auto i = _users.find(id);
	if (i == _users.end()) {
		static const std::string name = "NONAME";
		const UserPtr& u = std::make_shared<User>(peer, id, name, _messageSender, _world, _timeProvider, _containerProvider, _poiProvider);
		registerUser(u);
		return u;
	}
	const UserPtr& u = i->second;
	if (u->host() == peer->address.host) {
		Log::info("user %i reconnects with host %i on port %i", (int) id, peer->address.host, peer->address.port);
		i->second->setPeer(peer);
		return i->second;
	}

	Log::info("skip connection attempt for client %i - the hosts don't match", (int) id);
	return UserPtr();
}

bool EntityStorage::logout(EntityId userId) {
	auto i = _users.find(userId);
	if (i == _users.end())
		return false;
	_users.erase(i);
	return true;
}

void EntityStorage::addNpc(const NpcPtr& npc) {
	_npcs.insert(std::make_pair(npc->id(), std::move(npc)));
}

bool EntityStorage::removeNpc(ai::CharacterId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		return false;
	}
	_quadTree.remove(Node { i->second });
	_npcs.erase(id);
	return true;
}

NpcPtr EntityStorage::getNpc(ai::CharacterId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		Log::trace("Could not find npc with id %i", id);
		return NpcPtr();
	}
	return i->second;
}

void EntityStorage::onFrame(long dt) {
	static long lastFrame = _time;
	_time += dt;

	// let this run at 4 frames per second
	const long deltaLastTick = _time - lastFrame;
	const long delayBetweenTicks = 250L;
	if (deltaLastTick >= delayBetweenTicks) {
		lastFrame = _time - (deltaLastTick - delayBetweenTicks);
	} else {
		return;
	}

	_quadTreeCache.clear();
	updateQuadTree();
	for (auto i : _users) {
		updateEntity(i.second, deltaLastTick);
	}
	for (auto i = _npcs.begin(); i != _npcs.end();) {
		NpcPtr npc = i->second;
		if (!updateEntity(npc, deltaLastTick)) {
			Log::info("remove npc %i", npc->id());
			_quadTree.remove(Node { npc });
			i = _npcs.erase(i);
		} else {
			++i;
		}
	}
}

void EntityStorage::updateQuadTree() {
	// TODO: a full rebuild is not needed every frame
	_quadTree.clear();
	for (auto i : _npcs) {
		_quadTree.insert(Node { i.second });
	}
	for (auto i : _users) {
		const UserPtr& user = i.second;
		// this user is an ghost light
		_quadTree.insert(Node { user });
	}
}

bool EntityStorage::updateEntity(const EntityPtr& entity, long dt) {
	if (!entity->update(dt))
		return false;
	const core::RectFloat& rect = entity->regionRect();
	const auto& contents = _quadTreeCache.query(rect);
	EntitySet set;
	for (const Node& node : contents) {
		set.insert(node.entity);
	}
	set.erase(entity);
	entity->updateVisible(set);
	return true;
}

}
