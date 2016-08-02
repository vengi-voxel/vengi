/**
 * @file
 */

#include "ClientEntity.h"

namespace frontend {

ClientEntity::ClientEntity(ClientEntityId id, int type, const glm::vec3& pos, float orientation, const video::MeshPtr& mesh) :
		_id(id), _type(type), _orientation(orientation), _mesh(mesh) {
	_posLerp.setStartPosition(pos);
}

void ClientEntity::lerpPosition(const glm::vec3& position, float orientation) {
	_posLerp.setTargetPosition(position);
	_orientation = orientation;
}

void ClientEntity::update(long dt) {
	_posLerp.update(dt);
}

}
