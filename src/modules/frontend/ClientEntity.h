/**
 * @file
 */

#pragma once

#include "ClientEntityId.h"
#include "util/PosLerp.h"
#include <vector>
#include <functional>
#include "video/Mesh.h"
#include <memory>

namespace frontend {

class ClientEntity {
private:
	util::PosLerp _posLerp;
	ClientEntityId _id;
	int _type;
	float _orientation;
	video::MeshPtr _mesh;
public:
	ClientEntity(ClientEntityId id, int type, long now, const glm::vec3& pos, float orientation, const video::MeshPtr& mesh);

	void lerpPosition(long now, const glm::vec3& position, float orientation);
	void update(long now);

	inline int type() const {
		return _type;
	}

	inline ClientEntityId id() const {
		return _id;
	}

	inline float orientation() const {
		return _orientation;
	}

	inline const glm::vec3& position() const {
		return _posLerp.position();
	}

	inline float scale() const {
		return 1.0f;
	}

	inline const video::MeshPtr& mesh() const {
		return _mesh;
	}

	inline bool operator==(const ClientEntity& other) const {
		return _id == other._id;
	}
};

typedef std::shared_ptr<ClientEntity> ClientEntityPtr;

}

namespace std {
template<> struct hash<frontend::ClientEntity> {
	size_t operator()(const frontend::ClientEntity &c) const {
		return hash<int>()(c.id());
	}
};
}
