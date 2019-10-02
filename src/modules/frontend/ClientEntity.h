/**
 * @file
 */

#pragma once

#include "ServerMessages_generated.h"
#include "Shared_generated.h"
#include "ClientEntityId.h"
#include "attrib/ShadowAttributes.h"
#include "video/Buffer.h"
#include "animation/Character.h"
#include "animation/CharacterCache.h"
#include "AnimationShaders.h"
#include "stock/Stock.h"
#include <functional>
#include <vector>
#include <memory>

namespace frontend {

/**
 * @brief Entity representation for the client side.
 */
class ClientEntity {
private:
	ClientEntityId _id;
	network::EntityType _type;
	glm::vec3 _position {0};
	float _orientation;
	animation::Character _character;
	attrib::ShadowAttributes _attrib;
	stock::Stock _stock;
	animation::CharacterCachePtr _characterCache;
	video::Buffer _vbo;
	int32_t _vertices = -1;
	int32_t _indices = -1;
public:
	ClientEntity(const stock::StockDataProviderPtr& provider, const animation::CharacterCachePtr& characterCache, ClientEntityId id, network::EntityType type, const glm::vec3& pos, float orientation);
	~ClientEntity();

	void update(uint64_t dt);

	void setPosition(const glm::vec3& position);
	const glm::vec3& position() const;
	void setOrientation(float orientation);
	float orientation() const;

	bool operator==(const ClientEntity& other) const;

	uint32_t bindVertexBuffers(const shader::CharacterShader& chrShader);
	void unbindVertexBuffers();

	void setAnimation(animation::Animation animation);

	network::EntityType type() const;
	ClientEntityId id() const;

	// components
	stock::Stock& stock();
	attrib::ShadowAttributes& attrib();
	animation::Character& character();
};

inline void ClientEntity::setAnimation(animation::Animation animation) {
	character().setAnimation(animation);
}

inline stock::Stock& ClientEntity::stock() {
	return _stock;
}

inline animation::Character& ClientEntity::character() {
	return _character;
}

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

inline void ClientEntity::setOrientation(float orientation) {
	_orientation = orientation;
}

inline void ClientEntity::setPosition(const glm::vec3& position) {
	_position = position;
}

inline const glm::vec3& ClientEntity::position() const {
	return _position;
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
