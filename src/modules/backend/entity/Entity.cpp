/**
 * @file
 */

#include "Entity.h"
#include "core/Set.h"
#include "core/Common.h"

namespace backend {

void Entity::visibleAdd(const EntitySet& entities) {
	for (const EntityPtr& e : entities) {
		Log::trace("entity %i is visible for %i", (int)e->id(), (int)id());
	}
}

void Entity::visibleRemove(const EntitySet& entities) {
	for (const EntityPtr& e : entities) {
		Log::trace("entity %i is no longer visible for %i", (int)e->id(), (int)id());
	}
}

void Entity::initAttribs() {
	core_assert_always(_attribs.onFrame(0L));

	// the list of attribute types that should be set to max on spawn
	static const attrib::Type types[] = {
			attrib::Type::HEALTH,
			attrib::Type::SPEED,
			attrib::Type::VIEWDISTANCE,
			attrib::Type::ATTACKRANGE,
			attrib::Type::STRENGTH };

	for (size_t i = 0; i < SDL_arraysize(types); ++i) {
		const attrib::Type type = static_cast<attrib::Type>(types[i]);
		const double max = _attribs.max(type);
		Log::debug("Set current for %s to %f", network::EnumNameAttribType(type), max);
		_attribs.setCurrent(type, max);
		_dirtyTypes.insert(type);
	}
}

void Entity::onAttribChange(attrib::Type type, bool current, double value) {
	Log::debug("Attrib changed for type %s (current: %s) to value %f", network::EnumNameAttribType(type), (current ? "true" : "false"), value);
	_dirtyTypes.insert(type);
}

void Entity::addContainer(const std::string& id) {
	const attrib::ContainerPtr& c = _containerProvider->container(id);
	if (!c) {
		Log::error("could not add attribute container for %s", id.c_str());
		return;
	}
	_attribs.add(c);
}

void Entity::removeContainer(const std::string& id) {
	const attrib::ContainerPtr& c = _containerProvider->container(id);
	if (!c) {
		Log::error("could not remove attribute container for %s", id.c_str());
		return;
	}
	_attribs.remove(c);
}

void Entity::sendAttribUpdate() {
	// TODO: send current and max values to the clients
	// TODO: collect which of them are dirty, and maintain a list of
	// those that are for the owning client only or which of them must be broadcasted
	std::vector<ENetPeer*> peers;
	ENetPeer* p = peer();
	if (p != nullptr) {
		peers.push_back(p);
	}
	flatbuffers::FlatBufferBuilder fbb;
	auto attribs = fbb.CreateVector<flatbuffers::Offset<network::AttribEntry>>(_dirtyTypes.size(),
		[&] (size_t i) {
			attrib::Type type = *_dirtyTypes.erase(_dirtyTypes.begin());
			// TODO:
			double value = 0.0f;
			network::AttribMode mode = network::AttribMode::PERCENTAGE;
			bool current = false;
			return network::CreateAttribEntry(fbb, type, value, mode, current);
		});
	_messageSender->sendServerMessage(peers, fbb, network::ServerMsgType::AttribUpdate, network::CreateAttribUpdate(fbb, id(), attribs).Union());
}

bool Entity::update(long dt) {
	_attribs.onFrame(dt);
	if (!_dirtyTypes.empty()) {
		sendAttribUpdate();
		_dirtyTypes.clear();
	}
	_cooldowns.update();
	return true;
}

void Entity::updateVisible(const EntitySet& set) {
	_visibleLock.lockWrite();
	const auto& stillVisible = core::setIntersection(set, _visible);
	const EntitySet& remove = core::setDifference(stillVisible, _visible);
	const EntitySet& add = core::setDifference(set, stillVisible);
	_visible = core::setUnion(stillVisible, add);
	core_assert(stillVisible.size() + add.size() == _visible.size());
	_visibleLock.unlockWrite();

	ENetPeer* p = peer();
	if (p != nullptr) {
		for (const auto& e : stillVisible) {
			const glm::vec3& _pos = e->pos();
			const network::Vec3 posBuf {_pos.x, _pos.y, _pos.z};
			flatbuffers::FlatBufferBuilder fbb;
			_messageSender->sendServerMessage(p, fbb, network::ServerMsgType::EntityUpdate, CreateEntityUpdate(fbb, e->id(), &posBuf, e->orientation()).Union());
		}
	}

	if (!add.empty())
		visibleAdd(add);
	if (!remove.empty())
		visibleRemove(remove);
}

}
