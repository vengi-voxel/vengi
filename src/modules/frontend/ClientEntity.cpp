/**
 * @file
 */

#include "ClientEntity.h"

namespace frontend {

ClientEntity::ClientEntity(ClientEntityId id, int type, long now, const glm::vec3& pos, float orientation, const video::MeshPtr& mesh) :
		_id(id), _type(type), _orientation(orientation), _mesh(mesh) {
	_posLerp.setPosition(now, pos);
}

void ClientEntity::lerpPosition(long now, const glm::vec3& position, float orientation) {
	_posLerp.lerpPosition(now, position);
	_orientation = orientation;
}

void ClientEntity::update(long now) {
	_posLerp.update(now);
}

}
