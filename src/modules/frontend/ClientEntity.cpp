/**
 * @file
 */

#include "ClientEntity.h"

namespace frontend {

ClientEntity::ClientEntity(ClientEntityId id, network::messages::EntityType type, const glm::vec3& pos, float orientation, const video::MeshPtr& mesh) :
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

void ClientEntity::setCurrent(attrib::Types type, double value) {
	_attrib.setCurrent(type, value);
}

double ClientEntity::current(attrib::Types type) const {
	return _attrib.getCurrent(type);
}

void ClientEntity::update(long dt) {
	_posLerp.update(dt);
	_attrib.onFrame(dt);
}

}
