/**
 * @file
 */

#pragma once

#include "core/AppModule.h"
#include "voxel/World.h"
#include "network/MessageSender.h"
#include "server/Server.h"

#include "backend/poi/PoiProvider.h"
#include "backend/entity/EntityStorage.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/entity/ai/AILoader.h"
#include "backend/loop/ServerLoop.h"
#include "backend/spawn/SpawnMgr.h"
#include "backend/poi/PoiProvider.h"

#include "attrib/ContainerProvider.h"

using namespace backend;
using namespace core;
using namespace network;
using namespace voxel;
using namespace attrib;
using namespace io;

class ServerModule: public core::AbstractAppModule {
	void configureApp() const override {
		bind<Server>().in<di::SingletonScope>().to<Server(Network&, ServerLoop&, TimeProvider&, Filesystem&, EventBus&)>();
	}

	void configureBindings() const override {
		bind<PoiProvider>().in<di::SingletonScope>().to<PoiProvider(World&, TimeProvider&)>();
		bindSingleton<ContainerProvider>();
		bind<ServerLoop>().in<di::SingletonScope>().to<ServerLoop(Network&, SpawnMgr&, World&, EntityStorage&, EventBus&, AIRegistry&, ContainerProvider&, PoiProvider&)>();
		bindSingleton<AIRegistry>();
		bind<AILoader>().in<di::SingletonScope>().to<AILoader(AIRegistry&)>();
		bind<EntityStorage>().in<di::SingletonScope>().to<EntityStorage(MessageSender&, World&, TimeProvider&, ContainerProvider&, PoiProvider&)>();
		bind<SpawnMgr>().in<di::SingletonScope>().to<SpawnMgr(World&, EntityStorage&, MessageSender&, TimeProvider&, AILoader&, ContainerProvider&, PoiProvider&)>();
		bindSingleton<World>();
	}
};
