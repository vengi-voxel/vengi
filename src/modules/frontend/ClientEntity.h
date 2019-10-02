/**
 * @file
 */

#pragma once

#include "ServerMessages_generated.h"
#include "Shared_generated.h"
#include "ClientEntityId.h"
#include "util/PosLerp.h"
#include "attrib/ShadowAttributes.h"
#include "mesh/Mesh.h"
#include <functional>
#include <vector>
#include <memory>

namespace frontend {

/**
 * @brief Entity representation for the client side.
 */
class ClientEntity {
private:
	util::PosLerp _posLerp;
	ClientEntityId _id;
	network::EntityType _type;
	float _orientation;
	video::MeshPtr _mesh;
	attrib::ShadowAttributes _attrib;
public:
	ClientEntity(ClientEntityId id, network::EntityType type, const glm::vec3& pos, float orientation, const video::MeshPtr& mesh);
	~ClientEntity();

	void update(uint64_t dt);

	void lerpPosition(const glm::vec3& position, float orientation);
	const glm::vec3& position() const;
	float orientation() const;
	float scale() const;

	bool operator==(const ClientEntity& other) const;

	network::EntityType type() const;
	ClientEntityId id() const;

	// components
	attrib::ShadowAttributes& attrib();
	const video::MeshPtr& mesh() const;
};

inline attrib::ShadowAttributes& ClientEntity::attrib() {
	return _attrib;
}

inline network::EntityType ClientEntity::type() const {
	return _type;
}

inline ClientEntityId ClientEntity::id() const {
	return _id;
}

inline float ClientEntity::orientation() const {
	return _orientation;
}

inline const glm::vec3& ClientEntity::position() const {
	return _posLerp.position();
}

inline float ClientEntity::scale() const {
	return 1.0f;
}

inline const video::MeshPtr& ClientEntity::mesh() const {
	return _mesh;
}

inline bool ClientEntity::operator==(const ClientEntity& other) const {
	return _id == other._id;
}

typedef std::shared_ptr<ClientEntity> ClientEntityPtr;

}

namespace std {
template<> struct hash<frontend::ClientEntity> {
	size_t operator()(const frontend::ClientEntity &c) const {
		return hash<int>()(c.id());
	}
};
}
