#pragma once

#include <memory>
#include "entity/EntityId.h"

namespace voxel {

class World;
typedef std::shared_ptr<World> WorldPtr;

}

namespace ai {

class Zone;
class Server;

}

namespace eventmgr {

class EventMgr;
typedef std::shared_ptr<EventMgr> EventMgrPtr;

}

namespace cooldown {

class CooldownProvider;
typedef std::shared_ptr<CooldownProvider> CooldownProviderPtr;

}

namespace attrib {

class ContainerProvider;
typedef std::shared_ptr<ContainerProvider> ContainerProviderPtr;

}

namespace network {

class ServerMessageSender;
typedef std::shared_ptr<ServerMessageSender> ServerMessageSenderPtr;

}

namespace backend {

class AIRegistry;
typedef std::shared_ptr<AIRegistry> AIRegistryPtr;

class AILoader;
typedef std::shared_ptr<AILoader> AILoaderPtr;

class Entity;
typedef std::shared_ptr<Entity> EntityPtr;

class User;
typedef std::shared_ptr<User> UserPtr;

class Npc;
typedef std::shared_ptr<Npc> NpcPtr;

class EntityStorage;
typedef std::shared_ptr<EntityStorage> EntityStoragePtr;

}

namespace poi {

class PoiProvider;
typedef std::shared_ptr<PoiProvider> PoiProviderPtr;

}

namespace stock {

class StockDataProvider;
typedef std::shared_ptr<StockDataProvider> StockProviderPtr;

}

namespace eventmgr {

class EventMgr;
typedef std::shared_ptr<EventMgr> EventMgrPtr;

}

namespace core {

class TimeProvider;
typedef std::shared_ptr<TimeProvider> TimeProviderPtr;

}

namespace persistence {

class DBHandler;
typedef std::shared_ptr<DBHandler> DBHandlerPtr;

}
