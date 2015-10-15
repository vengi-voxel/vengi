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
	_attribs.onFrame(0L);

	static const attrib::Types types[] = {attrib::Types::HEALTH};

	for (size_t i = 0; i < lengthof(types); ++i) {
		const attrib::Types type = static_cast<attrib::Types>(types[i]);
		const double max = _attribs.getMax(type);
		_attribs.setCurrent(type, max);
	}
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
	_cooldowns.update();
	return true;
}

void Entity::updateVisible(const EntitySet& set) {
	_visibleLock.lockWrite();
	const auto& stillVisible = core::setIntersection(set, _visible);
	const EntitySet remove = std::move(core::setDifference(stillVisible, _visible));
	const EntitySet add = std::move(core::setDifference(set, stillVisible));
	_visible = std::move(core::setUnion(stillVisible, add));
	core_assert(stillVisible.size() + add.size() == _visible.size());
	_visibleLock.unlockWrite();

	ENetPeer* p = peer();
	if (p != nullptr) {
		for (const auto& e : stillVisible) {
			const glm::vec3 _pos = e->pos();
			const network::messages::Vec3 posBuf {_pos.x, _pos.y, _pos.z};
			flatbuffers::FlatBufferBuilder fbb;
			_messageSender->sendServerMessage(p, fbb, Type_NpcUpdate, CreateNpcUpdate(fbb, e->id(), &posBuf, e->orientation()).Union());
		}
	}

	if (!add.empty())
		visibleAdd(add);
	if (!remove.empty())
		visibleRemove(remove);
}

}
