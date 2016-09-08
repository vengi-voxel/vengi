/**
 * @file
 */

#pragma once

#include "network/MessageSender.h"
#include "Entity.h"
#include "core/Var.h"
#include "backend/poi/PoiProvider.h"

namespace backend {

class User : public Entity {
private:
	ENetPeer* _peer;
	std::string _name;
	std::string _password;
	std::string _email;
	glm::vec3 _pos;
	uint32_t _host;
	voxel::WorldPtr _world;
	PoiProviderPtr _poiProvider;
	network::MoveDirection _moveMask = network::MoveDirection::NONE;
	float _pitch;
	float _yaw;
	uint64_t _lastAction;
	uint64_t _time;
	core::VarPtr _userTimeout;
	flatbuffers::FlatBufferBuilder _entityUpdateFbb;

	inline bool isMove(network::MoveDirection dir) const {
		return (_moveMask & dir) != network::MoveDirection::NONE;
	}

	inline void addMove(network::MoveDirection dir) {
		_moveMask |= dir;
	}

	inline void removeMove(network::MoveDirection dir) {
		_moveMask &= ~dir;
	}

protected:
	void visibleAdd(const EntitySet& entities) override;
	void visibleRemove(const EntitySet& entities) override;

public:
	User(ENetPeer* peer, EntityId id, const std::string& name, const network::MessageSenderPtr& messageSender, const voxel::WorldPtr& world,
			const core::TimeProviderPtr& timeProvider, const attrib::ContainerProviderPtr& containerProvider, const PoiProviderPtr& poiProvider);

	void setEntityId(EntityId id) {
		_entityId = id;
	}

	void setPassword(const std::string& pw){
		_password = pw;
	}

	void setEmail(const std::string& mail){
		_email = mail;
	}

	inline const std::string& name() const {
		return _name;
	}

	inline const std::string& password() const {
		return _password;
	}

	inline const std::string& email() const {
		return _email;
	}

	void changeMovement(network::MoveDirection bitmask, float pitch, float yaw) {
		_moveMask = bitmask;
		_pitch = pitch;
		_yaw = yaw;
	}

	void attack(EntityId id);

	/**
	 * @brief The client closed the connection - the user object itself will stay in the server until
	 * a logout cooldown was hit
	 */
	void disconnect();

	void reconnect();

	bool update(long dt) override;

	/**
	 * @brief Sets a new ENetPeer and returns the old one.
	 */
	ENetPeer* setPeer(ENetPeer* peer);

	uint32_t host() const {
		return _host;
	}

	const glm::vec3& pos() const override {
		return _pos;
	}

	void setPos(const glm::vec3& pos);

	float orientation() const override {
		return _pitch;
	}

	/**
	 * @brief Informs the user that the login was successful
	 */
	void sendUserSpawn() const;
	void sendSeed(long seed) const;
};

typedef std::shared_ptr<User> UserPtr;

}
