/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "network/Network.h"
#include "core/TimeProvider.h"
#include "ai/common/Types.h"
#include "UserModel.h"
#include <unordered_map>

namespace backend {

/**
 * @brief Manages the Entity instances of the backend.
 *
 * This includes calling the Entity::update() method as well as performing the visibility calculations.
 */
class EntityStorage {
private:
	typedef std::unordered_map<EntityId, UserPtr> Users;
	typedef Users::iterator UsersIter;
	Users _users;

	typedef std::unordered_map<ai::CharacterId, NpcPtr> Npcs;
	typedef Npcs::iterator NpcsIter;
	Npcs _npcs;

	network::ServerMessageSenderPtr _messageSender;
	core::TimeProviderPtr _timeProvider;
	attrib::ContainerProviderPtr _containerProvider;
	poi::PoiProviderPtr _poiProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	persistence::DBHandlerPtr _dbHandler;
	stock::StockProviderPtr _stockDataProvider;
	core::EventBusPtr _eventBus;
	MapProviderPtr _mapProvider;
	long _time;

	void registerUser(const UserPtr& user);
	bool updateEntity(const EntityPtr& entity, long dt);

	bool userData(db::UserModel& model, const std::string& email, const std::string& password) const;
public:
	EntityStorage(const MapProviderPtr& mapProvider, const network::ServerMessageSenderPtr& messageSender, const core::TimeProviderPtr& timeProvider,
			const attrib::ContainerProviderPtr& containerProvider, const poi::PoiProviderPtr& poiProvider, const cooldown::CooldownProviderPtr& cooldownProvider,
			const persistence::DBHandlerPtr& dbHandler, const stock::StockProviderPtr& stockDataProvider, const core::EventBusPtr& eventBus);

	UserPtr login(ENetPeer* peer, const std::string& email, const std::string& password);
	bool logout(EntityId userId);

	void addNpc(const NpcPtr& npc);
	bool removeNpc(ai::CharacterId id);
	NpcPtr npc(ai::CharacterId id);

	void update(long dt);
};

typedef std::shared_ptr<EntityStorage> EntityStoragePtr;

}
