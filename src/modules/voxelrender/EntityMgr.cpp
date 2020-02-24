/**
 * @file
 */

#include "EntityMgr.h"

namespace voxelrender {

EntityMgr::EntityMgr() :
		_visibleEntities(1024) {
}

void EntityMgr::reset() {
	_entities.clear();
}

void EntityMgr::update(uint64_t dt) {
	for (const auto& e : _entities) {
		e->value->update(dt);
	}
}

void EntityMgr::updateVisibleEntities(uint64_t deltaFrame, const video::Camera& camera) {
	_visibleEntities.clear();
	for (const auto& e : _entities) {
		const frontend::ClientEntityPtr& ent = e->value;
		ent->update(deltaFrame);
		// note, that the aabb does not include the orientation - that should be kept in mind here.
		// a particular rotation could lead to an entity getting culled even though it should still
		// be visible.
		math::AABB<float> aabb = ent->character().aabb();
		aabb.shift(ent->position());
		if (!camera.isVisible(aabb)) {
			continue;
		}
		_visibleEntities.insert(ent.get());
	}
}

frontend::ClientEntityPtr EntityMgr::getEntity(frontend::ClientEntityId id) const {
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		Log::warn("Could not get entity with id %li", id);
		return frontend::ClientEntityPtr();
	}
	return i->second;
}

bool EntityMgr::addEntity(const frontend::ClientEntityPtr& entity) {
	auto i = _entities.find(entity->id());
	if (i != _entities.end()) {
		return false;
	}
	_entities.put(entity->id(), entity);
	return true;
}

bool EntityMgr::removeEntity(frontend::ClientEntityId id) {
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		return false;
	}
	_entities.erase(i);
	return true;
}

}