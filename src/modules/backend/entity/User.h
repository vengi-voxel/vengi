/**
 * @file
 */

#pragma once

#include "backend/network/ServerMessageSender.h"
#include "Entity.h"
#include "core/Var.h"
#include "user/UserAttribMgr.h"
#include "user/UserStockMgr.h"
#include "user/UserCooldownMgr.h"
#include "user/UserLogoutMgr.h"
#include "user/UserMovementMgr.h"
#include "persistence/DBHandler.h"
#include "stock/StockDataProvider.h"

namespace backend {

class User : public Entity {
private:
	using Super = Entity;
	core::String _name;
	core::String _email;
	persistence::DBHandlerPtr _dbHandler;
	core::TimeProviderPtr _timeProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	core::StringMap<core::String> _userinfo;

	UserStockMgr _stockMgr;
	UserCooldownMgr _cooldownMgr;
	UserAttribMgr _attribMgr;
	UserLogoutMgr _logoutMgr;
	UserMovementMgr _movementMgr;

public:
	User(ENetPeer* peer,
			EntityId id,
			const core::String& name,
			const MapPtr& map,
			const network::ServerMessageSenderPtr& messageSender,
			const core::TimeProviderPtr& timeProvider,
			const attrib::ContainerProviderPtr& containerProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider,
			const persistence::DBHandlerPtr& dbHandler,
			const persistence::PersistenceMgrPtr& persistenceMgr,
			const stock::StockDataProviderPtr& stockDataProvider);
	~User();

	void setEntityId(EntityId id);

	const core::String& email() const;
	void setEmail(const core::String& mail);

	const core::String& name() const;

	/**
	 * @brief Called for every connect/reconnect
	 * @sa onReconnect()
	 */
	void onConnect();
	/**
	 * @brief Called only for reconnects. This is called before onConnect() is called.
	 * @sa onConnect()
	 */
	void onReconnect();

	/**
	 * @brief Sets the user info values that are set on the client side and broadcasted to
	 * other players (see @c core::CV_BROADCAST)
	 */
	void userinfo(const char *key, const char* value);

	bool update(long dt) override;

	void init() override;
	void shutdown() override;

	/**
	 * @brief Sets a new ENetPeer and returns the old one.
	 */
	ENetPeer* setPeer(ENetPeer* peer);

	bool sendMessage(flatbuffers::FlatBufferBuilder& fbb, network::ServerMsgType type, flatbuffers::Offset<void> msg) const;

	/**
	 * @brief Informs the user that the login was successful
	 */
	void broadcastUserSpawn() const;
	/**
	 * @brief Send all replicate vars from the server to the user
	 */
	void sendVars() const;
	/**
	 * @brief Send the userinfo to all visible users
	 */
	void broadcastUserinfo();

	UserLogoutMgr& logoutMgr();
	const UserLogoutMgr& logoutMgr() const;

	UserCooldownMgr& cooldownMgr();
	const UserCooldownMgr& cooldownMgr() const;

	UserMovementMgr& movementMgr();
	const UserMovementMgr& movementMgr() const;
};

inline UserLogoutMgr& User::logoutMgr() {
	return _logoutMgr;
}

inline const UserLogoutMgr& User::logoutMgr() const {
	return _logoutMgr;
}

inline UserMovementMgr& User::movementMgr() {
	return _movementMgr;
}

inline const UserMovementMgr& User::movementMgr() const {
	return _movementMgr;
}

inline UserCooldownMgr& User::cooldownMgr() {
	return _cooldownMgr;
}

inline const UserCooldownMgr& User::cooldownMgr() const {
	return _cooldownMgr;
}

inline void User::setEntityId(EntityId id) {
	_entityId = id;
}

inline void User::setEmail(const core::String& mail) {
	_email = mail;
}

inline const core::String& User::name() const {
	return _name;
}

inline const core::String& User::email() const {
	return _email;
}

typedef std::shared_ptr<User> UserPtr;

}
