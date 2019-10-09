/**
 * @file
 */

#include "ClientEntity.h"

namespace frontend {

ClientEntity::ClientEntity(ClientEntityId id, network::EntityType type, const glm::vec3& pos, float orientation, const mesh::MeshPtr& mesh) :
		_id(id), _type(type), _orientation(orientation), _mesh(mesh) {
	_posLerp.setStartPosition(pos);
}

ClientEntity::~ClientEntity() {
	_mesh->shutdown();
}

void ClientEntity::lerpPosition(const glm::vec3& position, float orientation) {
	_posLerp.setTargetPosition(position);
	_orientation = orientation;
}

void ClientEntity::update(uint64_t dt) {
	_posLerp.update(dt);
	_attrib.update(dt);
}

}
