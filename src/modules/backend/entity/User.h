#pragma once

#include "network/MessageSender.h"
#include "Npc.h"
#include "core/Var.h"

namespace backend {

class User : public Entity {
private:
	ENetPeer* _peer;
	std::string _name;
	std::string _password;
	std::string _email;
	glm::vec3 _pos;
	NpcPtr _npc;
	uint32_t _host;
	voxel::WorldPtr _world;
	PoiProviderPtr _poiProvider;
	MoveDirection _moveMask;
	float _pitch;
	float _yaw;
	uint64_t _lastAction;
	uint64_t _time;
	core::VarPtr _userTimeout;

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

	void changeMovement(MoveDirection bitmask, float pitch, float yaw) {
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

	bool update(long dt) override;

	ENetPeer* peer() const override {
		if (_peer->state == ENET_PEER_STATE_DISCONNECTED)
			return nullptr;
		return _peer;
	}

	/**
	 * @brief Sets a new ENetPeer and returns the old one.
	 */
	ENetPeer* setPeer(ENetPeer* peer);

	uint32_t host() const {
		return _host;
	}

	glm::vec3 pos() const override {
		return _pos;
	}

	void setPos(const glm::vec3& pos);

	float orientation() const override {
		return _pitch;
	}

	network::messages::NpcType npcType() const override {
		if (!_npc)
			return network::messages::NpcType_NONE;
		return _npc->npcType();
	}

	inline bool hasTakenOverNpc() const {
		const bool val = _npc.get() != nullptr;
		return val;
	}

	inline NpcPtr takenOverNpc() const {
		return _npc;
	}

	bool takeOverNpc(const NpcPtr& character);
	/**
	 * @brief Informs the user that the login was successful
	 */
	void sendUserSpawn() const;
	void sendUserUpdate() const;
	void sendEntityUpdate(const EntityPtr& entity) const;
	void sendEntitySpawn(const EntityPtr& entity) const;
	void sendEntityRemove(const EntityPtr& entity) const;
	void sendSeed(long seed) const;
};

typedef std::shared_ptr<User> UserPtr;

}
