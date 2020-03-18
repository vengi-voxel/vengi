/**
 * @file
 */

#pragma once

#include "core/collection/Map.h"
#include "core/collection/List.h"
#include "frontend/ClientEntity.h"
#include "video/Camera.h"

namespace frontend {

class EntityMgr {
private:
	typedef core::Map<frontend::ClientEntityId, frontend::ClientEntityPtr, 128> Entities;
	Entities _entities;
	core::List<frontend::ClientEntity*> _visibleEntities;

public:
	EntityMgr();

	void updateVisibleEntities(uint64_t deltaFrame, const video::Camera& camera);

	void reset();
	void update(uint64_t dt);

	frontend::ClientEntityPtr getEntity(frontend::ClientEntityId id) const;
	bool addEntity(const frontend::ClientEntityPtr &entity);
	bool removeEntity(frontend::ClientEntityId id);

	const core::List<frontend::ClientEntity*>& visibleEntities() const;
};

inline const core::List<frontend::ClientEntity*>& EntityMgr::visibleEntities() const {
	return _visibleEntities;
}

}