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
	static const attrib::Types types[] = {
			attrib::Types::HEALTH,
			attrib::Types::SPEED,
			attrib::Types::VIEWDISTANCE,
			attrib::Types::ATTACKRANGE,
			attrib::Types::STRENGTH };

	for (size_t i = 0; i < SDL_arraysize(types); ++i) {
		const attrib::Types type = static_cast<attrib::Types>(types[i]);
		const double max = _attribs.getMax(type);
		Log::debug("Set current for %s to %f", network::EnumNameAttribType(type), max);
		_attribs.setCurrent(type, max);
		_dirtyTypes.insert(type);
	}
}

void Entity::onAttribChange(attrib::Types type) {
	_dirtyTypes.insert(type);
}

void Entity::addContainer(const std::string& id) {
	const attrib::ContainerPtr& c = _containerProvider->getContainer(id);
	if (!c) {
		Log::error("could not add attribute container for %s", id.c_str());
		return;
	}
	_attribs.add(c);
}

void Entity::removeContainer(const std::string& id) {
	const attrib::ContainerPtr& c = _containerProvider->getContainer(id);
	if (!c) {
		Log::error("could not remove attribute container for %s", id.c_str());
		return;
	}
	_attribs.remove(c);
}

bool Entity::update(long dt) {
	_attribs.onFrame(dt);
	if (!_dirtyTypes.empty()) {
		// TODO: send current and max values to the clients
		// TODO: collect which of them are dirty, and maintain a list of
		// those that are for the owning client only or which of them must be broadcasted
#if 0
		ENetPeer* p = peer();
		if (p != nullptr) {
			flatbuffers::FlatBufferBuilder fbb;
			std::vector<flatbuffers::Offset<network::AttribEntry>> *attribs;
			_messageSender->sendServerMessage(p, fbb, network::ServerMsgType::AttribUpdate, CreateAttribUpdate(fbb, id(), attribs).Union());
		}
#endif
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
