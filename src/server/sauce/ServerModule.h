#pragma once

#include <sauce/sauce.h>

#include "core/AbstractModule.h"
#include "voxel/World.h"
#include "network/MessageSender.h"
#include "Server.h"

#include "backend/poi/PoiProvider.h"
#include "backend/entity/EntityStorage.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/entity/ai/AILoader.h"
#include "backend/loop/ServerLoop.h"
#include "backend/spawn/SpawnMgr.h"
#include "backend/poi/PoiProvider.h"

#include "attrib/ContainerProvider.h"

using namespace backend;
using namespace sauce;
using namespace core;
using namespace network;
using namespace voxel;
using namespace attrib;
using namespace io;

class ServerModule: public core::AbstractModule {
	void configure() const override {
		core::AbstractModule::configure();
		bind<Server>().in<SingletonScope>().to<Server(Network&, ServerLoop&, TimeProvider&, Filesystem&, EventBus&)>();

		bind<PoiProvider>().in<SingletonScope>().to<PoiProvider(World&, TimeProvider&)>();
		bind<ContainerProvider>().in<SingletonScope>().to<ContainerProvider>();
		bind<ServerLoop>().in<SingletonScope>().to<ServerLoop(Network&, SpawnMgr&, World&, EntityStorage&, EventBus&, AIRegistry&, ContainerProvider&, PoiProvider&)>();
		bind<AIRegistry>().in<SingletonScope>().to<AIRegistry>();
		bind<AILoader>().in<SingletonScope>().to<AILoader(AIRegistry&)>();
		bind<EntityStorage>().in<SingletonScope>().to<EntityStorage(MessageSender&, World&, TimeProvider&, ContainerProvider&, PoiProvider&)>();
		bind<SpawnMgr>().in<SingletonScope>().to<SpawnMgr(World&, EntityStorage&, MessageSender&, TimeProvider&, AILoader&, ContainerProvider&, PoiProvider&)>();
		bind<World>().in<SingletonScope>().to<World>();
	}
};
