/**
 * @file
 */

#pragma once

#include "Shared_generated.h"
#include "ClientEntityId.h"
#include "attrib/ShadowAttributes.h"
#include "video/Buffer.h"
#include "animation/Animation.h"
#include "animation/chr/Character.h"
#include "stock/Stock.h"
#include "core/collection/Array.h"
#include "core/collection/StringMap.h"
#include "SkeletonShaderConstants.h"
#include "core/SharedPtr.h"
#include <memory>

namespace shader {
class SkeletonShader;
}

namespace animation {
class AnimationCache;
using AnimationCachePtr = std::shared_ptr<AnimationCache>;
}

namespace frontend {

/**
 * @brief Entity representation for the client side.
 */
class ClientEntity {
private:
	core::Array<glm::mat4, shader::SkeletonShaderConstants::getMaxBones()> _bones;
	ClientEntityId _id;
	network::EntityType _type;
	glm::vec3 _position {0};
	glm::mat4 _model { 1.0f };
	float _orientation;
	animation::Character _character;
	attrib::ShadowAttributes _attrib;
	stock::Stock _stock;
	animation::AnimationCachePtr _animationCache;
	video::Buffer _vbo;
	int32_t _vertices = -1;
	int32_t _indices = -1;
	core::StringMap<core::String> _userinfo;
public:
	ClientEntity(const stock::StockDataProviderPtr& provider, const animation::AnimationCachePtr& animationCache,
			ClientEntityId id, network::EntityType type, const glm::vec3& pos, float orientation);
	~ClientEntity();

	void update(double deltaFrameSeconds);

	void setPosition(const glm::vec3& position);
	const glm::vec3& position() const;
	void setOrientation(float orientation);
	float orientation() const;
	void userinfo(const core::String& key, const core::String& value);

	const glm::mat4& modelMatrix() const;
	const core::Array<glm::mat4, shader::SkeletonShaderConstants::getMaxBones()> bones() const;

	bool operator==(const ClientEntity& other) const;

	uint32_t bindVertexBuffers(const shader::SkeletonShader& chrShader);
	void unbindVertexBuffers();

	void setAnimation(animation::Animation animation, bool reset);
	void addAnimation(animation::Animation animation, double durationSeconds);

	network::EntityType type() const;
	ClientEntityId id() const;

	// components
	stock::Stock& stock();
	attrib::ShadowAttributes& attrib();
	animation::Character& character();
};

inline const core::Array<glm::mat4, shader::SkeletonShaderConstants::getMaxBones()> ClientEntity::bones() const {
	return _bones;
}

inline const glm::mat4& ClientEntity::modelMatrix() const {
	return _model;
}

inline void ClientEntity::setAnimation(animation::Animation animation, bool reset) {
	character().setAnimation(animation, reset);
}

inline void ClientEntity::addAnimation(animation::Animation animation, double durationSeconds) {
	character().addAnimation(animation, durationSeconds);
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

inline const glm::vec3& ClientEntity::position() const {
	return _position;
}

inline bool ClientEntity::operator==(const ClientEntity& other) const {
	return _id == other._id;
}

typedef core::SharedPtr<ClientEntity> ClientEntityPtr;

}

namespace std {
template<> struct hash<frontend::ClientEntity> {
	size_t operator()(const frontend::ClientEntity &c) const {
		return hash<int>()(c.id());
	}
};
}
