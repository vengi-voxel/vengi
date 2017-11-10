/**
 * @file
 */

#pragma once

#include "network/ServerMessageSender.h"
#include "Entity.h"
#include "core/Var.h"
#include "user/UserStockMgr.h"
#include "user/UserCooldownMgr.h"
#include "persistence/DBHandler.h"
#include "stock/StockDataProvider.h"

namespace backend {

/**
 * @todo move everything into dedicated components
 */
class User : public Entity {
private:
	using Super = Entity;
	std::string _name;
	std::string _email;
	persistence::DBHandlerPtr _dbHandler;
	network::MoveDirection _moveMask = network::MoveDirection::NONE;
	float _yaw = 0.0f;
	bool _disconnect = false;
	uint64_t _lastAction = 0u;
	uint64_t _time = 0u;
	core::VarPtr _userTimeout;
	flatbuffers::FlatBufferBuilder _entityUpdateFBB;
	mutable flatbuffers::FlatBufferBuilder _cooldownFBB;

	UserStockMgr _stockMgr;
	core::TimeProviderPtr _timeProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	UserCooldownMgr _cooldownMgr;

	bool isMove(network::MoveDirection dir) const;
	void addMove(network::MoveDirection dir);
	void removeMove(network::MoveDirection dir);

	void onCooldownExpired(cooldown::Type type);
	void triggerCooldown(cooldown::Type type);

public:
	User(ENetPeer* peer,
			EntityId id,
			const std::string& name,
			const MapPtr& map,
			const network::ServerMessageSenderPtr& messageSender,
			const core::TimeProviderPtr& timeProvider,
			const attrib::ContainerProviderPtr& containerProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider,
			const persistence::DBHandlerPtr& dbHandler,
			const stock::StockProviderPtr& stockDataProvider);
	~User();

	void setEntityId(EntityId id);

	void setEmail(const std::string& mail);

	const std::string& name() const;

	const std::string& email() const;

	void changeMovement(network::MoveDirection bitmask, float pitch, float yaw);

	void attack(EntityId id);

	/**
	 * @brief The client wants to disconnect - the user object itself will stay in the server until
	 * a logout cooldown was hit
	 */
	void triggerLogout();

	void reconnect();

	void init() override;
	bool update(long dt) override;

	/**
	 * @brief Sets a new ENetPeer and returns the old one.
	 */
	ENetPeer* setPeer(ENetPeer* peer);

	void shutdown();

	/**
	 * @brief Informs the user that the login was successful
	 */
	void sendUserSpawn() const;
	void sendSeed(long seed) const;
	void sendCooldown(cooldown::Type type, bool started) const;
};

inline bool User::isMove(network::MoveDirection dir) const {
	return (_moveMask & dir) != network::MoveDirection::NONE;
}

inline void User::addMove(network::MoveDirection dir) {
	_moveMask |= dir;
}

inline void User::removeMove(network::MoveDirection dir) {
	_moveMask &= ~dir;
}

inline void User::setEntityId(EntityId id) {
	_entityId = id;
}

inline void User::setEmail(const std::string& mail) {
	_email = mail;
}

inline const std::string& User::name() const {
	return _name;
}

inline const std::string& User::email() const {
	return _email;
}

inline void User::changeMovement(network::MoveDirection bitmask, float pitch, float yaw) {
	_moveMask = bitmask;
	_orientation = pitch;
	_yaw = yaw;
}

typedef std::shared_ptr<User> UserPtr;

}
