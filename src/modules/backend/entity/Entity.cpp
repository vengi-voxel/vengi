/**
 * @file
 */

#include "Entity.h"
#include "core/collection/Set.h"
#include "core/ArrayLength.h"
#include "core/Assert.h"
#include "math/Rect.h"
#include "core/Common.h"
#include "math/Frustum.h"
#include "backend/world/Map.h"
#include "poi/PoiProvider.h"
#include "network/ServerMessageSender.h"
#include "attrib/ContainerProvider.h"

namespace backend {

Entity::Entity(EntityId id,
		const MapPtr& map,
		const network::ServerMessageSenderPtr& messageSender,
		const core::TimeProviderPtr& timeProvider,
		const attrib::ContainerProviderPtr& containerProvider) :
		_messageSender(messageSender), _containerProvider(containerProvider),
		_map(map), _entityId(id) {
	_attribs.addListener(std::bind(&Entity::onAttribChange, this, std::placeholders::_1));
}

Entity::~Entity() {
}

void Entity::visibleAdd(const EntitySet& entities) {
	for (const EntityPtr& e : entities) {
		Log::trace("entity %i is visible for %i", (int)e->id(), (int)id());
		sendEntitySpawn(e);
	}
}

void Entity::visibleRemove(const EntitySet& entities) {
	for (const EntityPtr& e : entities) {
		Log::trace("entity %i is no longer visible for %i", (int)e->id(), (int)id());
		sendEntityRemove(e);
	}
}

void Entity::setPointOfInterest(poi::Type type) {
	map()->poiProvider()->add(pos(), type);
}

const char* Entity::type() const {
	return network::EnumNameEntityType(_entityType);
}

bool Entity::attack(EntityId victimId) {
	return map()->attackMgr().startAttack(id(), victimId);
}

void Entity::sendToVisible(flatbuffers::FlatBufferBuilder& fbb, network::ServerMsgType type,
		flatbuffers::Offset<void> data, bool sendToSelf, uint32_t flags) const {
	const EntitySet& visible = visibleCopy();
	std::vector<ENetPeer*> peers;
	peers.reserve(visible.size() + 1);
	if (sendToSelf) {
		ENetPeer* p = peer();
		if (p != nullptr) {
			peers.push_back(p);
		}
	}
	for (const EntityPtr& e : visible) {
		ENetPeer* peer = e->peer();
		if (peer == nullptr) {
			continue;
		}
		peers.push_back(peer);
	}
	if (peers.empty()) {
		return;
	}
	_messageSender->sendServerMessage(&peers[0], peers.size(), fbb, type, data, flags);
}

void Entity::init() {
	const char *typeName = network::EnumNameEntityType(_entityType);
	addContainer(typeName);

	core_assert_always(_attribs.update(0L));

	// the list of attribute types that should be set to max on spawn
	static const attrib::Type types[] = {
			attrib::Type::HEALTH,
			attrib::Type::SPEED,
			attrib::Type::VIEWDISTANCE,
			attrib::Type::ATTACKRANGE,
			attrib::Type::STRENGTH };

	for (int i = 0; i < lengthof(types); ++i) {
		const attrib::Type type = static_cast<attrib::Type>(types[i]);
		const double max = _attribs.max(type);
		Log::debug("Set current for %s to %f", network::EnumNameAttribType(type), max);
		_attribs.setCurrent(type, max);
	}
}

void Entity::shutdown() {
}

void Entity::onAttribChange(const attrib::DirtyValue& v) {
	Log::debug("Attrib changed for type %s (current: %s) to value %f",
			network::EnumNameAttribType(v.type), (v.current ? "true" : "false"), v.value);
	_dirtyAttributeTypes.insert(v);
}

bool Entity::addContainer(const std::string& id) {
	const attrib::ContainerPtr& c = _containerProvider->container(id);
	if (!c) {
		Log::error("could not add attribute container for %s", id.c_str());
		return false;
	}
	_attribs.add(c);
	return true;
}

bool Entity::removeContainer(const std::string& id) {
	const attrib::ContainerPtr& c = _containerProvider->container(id);
	if (!c) {
		Log::error("could not remove attribute container for %s", id.c_str());
		return false;
	}
	_attribs.remove(c);
	return true;
}

void Entity::broadcastAttribUpdate() {
	// TODO: send current and max values to the clients
	// TODO: collect which of them are dirty, and maintain a list of
	// those that are for the owning client only or which of them must be broadcasted
	_attribUpdateFBB.Clear();
	auto iter = _dirtyAttributeTypes.begin();
	auto attribs = _attribUpdateFBB.CreateVector<flatbuffers::Offset<network::AttribEntry>>(_dirtyAttributeTypes.size(),
		[&] (size_t i) {
			const attrib::DirtyValue& dirtyValue = *iter++;
			const double value = dirtyValue.value;
			// TODO: maybe not needed?
			const network::AttribMode mode = network::AttribMode::Percentage;
			const bool current = dirtyValue.current;
			return network::CreateAttribEntry(_attribUpdateFBB, dirtyValue.type, value, mode, current);
		});
	sendToVisible(_attribUpdateFBB, network::ServerMsgType::AttribUpdate,
			network::CreateAttribUpdate(_attribUpdateFBB, id(), attribs).Union(), true);
}

bool Entity::update(long dt) {
	_attribs.update(dt);
	if (!_dirtyAttributeTypes.empty()) {
		broadcastAttribUpdate();
		_dirtyAttributeTypes.clear();
	}
	return true;
}

void Entity::updateVisible(const EntitySet& set) {
	_visibleLock.lockWrite();
	const auto& stillVisible = core::setIntersection(set, _visible);
	const EntitySet& remove = core::setDifference(stillVisible, _visible);
	const EntitySet& add = core::setDifference(set, stillVisible);
	_visible = core::setUnion(stillVisible, add);
	_visibleLock.unlockWrite();

	for (const auto& e : _visible) {
		sendEntityUpdate(e);
	}

	if (!add.empty()) {
		visibleAdd(add);
	}
	if (!remove.empty()) {
		visibleRemove(remove);
	}
}

void Entity::sendEntityUpdate(const EntityPtr& entity) const {
	if (_peer == nullptr) {
		return;
	}
	const glm::vec3& _pos = entity->pos();
	const network::Vec3 pos { _pos.x, _pos.y, _pos.z };
	_entityUpdateFBB.Clear();
	_messageSender->sendServerMessage(_peer, _entityUpdateFBB, network::ServerMsgType::EntityUpdate,
			network::CreateEntityUpdate(_entityUpdateFBB, entity->id(), &pos, entity->orientation(), entity->animation()).Union());
}

void Entity::sendEntitySpawn(const EntityPtr& entity) const {
	if (_peer == nullptr) {
		return;
	}
	const glm::vec3& pos = entity->pos();
	const network::Vec3 vec3 { pos.x, pos.y, pos.z };
	const EntityId entityId = id();
	_entitySpawnFBB.Clear();
	// TODO: User::sendUserSpawn()?
	_messageSender->sendServerMessage(_peer, _entitySpawnFBB, network::ServerMsgType::EntitySpawn,
			network::CreateEntitySpawn(_entitySpawnFBB, entity->id(), entity->entityType(), &vec3, entityId, entity->animation()).Union());
}

void Entity::sendEntityRemove(const EntityPtr& entity) const {
	if (_peer == nullptr) {
		return;
	}
	_entityRemoveFBB.Clear();
	_messageSender->sendServerMessage(_peer, _entityRemoveFBB, network::ServerMsgType::EntityRemove,
			network::CreateEntityRemove(_entityRemoveFBB, entity->id()).Union());
}

bool Entity::inFrustum(const glm::vec3& position) const {
	const double fieldOfView = current(attrib::Type::FIELDOFVIEW);
	if (fieldOfView <= 1.0) {
		return false;
	}
	return math::Frustum::isVisible(pos(), orientation(), position, glm::radians(fieldOfView));
}

math::RectFloat Entity::rect() const {
	const glm::vec3 p = pos();
	const float halfSize = size() / 2.0f;
	return math::RectFloat(p.x - halfSize, p.z - halfSize, p.x + halfSize, p.z + halfSize);
}

math::RectFloat Entity::viewRect() const {
	const glm::vec3 p = pos();
	const float viewDistance = current(attrib::Type::VIEWDISTANCE);
	core_assert_msg(viewDistance > 0.0f, "Expected to get a view distance > 0.0f, but got %f (EntityType: %i)", viewDistance, (int)entityType());
	return math::RectFloat(p.x - viewDistance, p.z - viewDistance, p.x + viewDistance, p.z + viewDistance);
}

}
