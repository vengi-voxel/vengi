#pragma once

#include <memory>
#include "entity/EntityId.h"

namespace voxelworld {

class WorldMgr;
typedef std::shared_ptr<WorldMgr> WorldMgrPtr;

}

namespace voxelformat {

class VolumeCache;
typedef std::shared_ptr<VolumeCache> VolumeCachePtr;

}

namespace ai {

class Zone;
class Server;
class AIRegistry;
class TreeNode;
typedef std::shared_ptr<TreeNode> TreeNodePtr;

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
typedef std::shared_ptr<ContainerProvider> ContainerProviderPtr;

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
