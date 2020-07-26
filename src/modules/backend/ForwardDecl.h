#pragma once

#include <memory>
#include "entity/EntityId.h"
#include "core/SharedPtr.h"

namespace voxelworld {

class WorldMgr;
typedef std::shared_ptr<WorldMgr> WorldMgrPtr;

class WorldPager;
typedef core::SharedPtr<WorldPager> WorldPagerPtr;

}

namespace voxelformat {

class VolumeCache;
typedef std::shared_ptr<VolumeCache> VolumeCachePtr;

}

namespace backend {

class Zone;
class Server;
class AIRegistry;
class TreeNode;
typedef std::shared_ptr<TreeNode> TreeNodePtr;

class ICharacter;
typedef core::SharedPtr<ICharacter> ICharacterPtr;
class Zone;

class AI;
typedef std::shared_ptr<AI> AIPtr;

class AICharacter;
typedef core::SharedPtr<AICharacter> AICharacterPtr;

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

class Attributes;

class ContainerProvider;
typedef core::SharedPtr<ContainerProvider> ContainerProviderPtr;

}

namespace network {

class ServerMessageSender;
typedef std::shared_ptr<ServerMessageSender> ServerMessageSenderPtr;

}

namespace backend {

class World;
typedef std::shared_ptr<World> WorldPtr;

class MetricMgr;
typedef std::shared_ptr<MetricMgr> MetricMgrPtr;

class Map;
typedef std::shared_ptr<Map> MapPtr;

class MapProvider;
typedef std::shared_ptr<MapProvider> MapProviderPtr;

class SpawnMgr;
typedef std::shared_ptr<SpawnMgr> SpawnMgrPtr;

class LUAAIRegistry;
typedef std::shared_ptr<LUAAIRegistry> AIRegistryPtr;

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

}

namespace stock {

class StockDataProvider;
typedef std::shared_ptr<StockDataProvider> StockDataProviderPtr;

}

namespace eventmgr {

class EventMgr;
typedef std::shared_ptr<EventMgr> EventMgrPtr;

}

namespace core {

class TimeProvider;
typedef std::shared_ptr<TimeProvider> TimeProviderPtr;

class EventBus;
typedef std::shared_ptr<EventBus> EventBusPtr;

}

namespace io {

class Filesystem;
typedef std::shared_ptr<Filesystem> FilesystemPtr;

}

namespace persistence {

class DBHandler;
typedef std::shared_ptr<DBHandler> DBHandlerPtr;

class PersistenceMgr;
typedef std::shared_ptr<PersistenceMgr> PersistenceMgrPtr;

}
