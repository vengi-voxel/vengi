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
#ifdef DI_SAUCE
		bind<Server>().in<di::SingletonScope>().to<Server(Network&, ServerLoop&, TimeProvider&, Filesystem&, EventBus&)>();
#endif
#ifdef DI_BOOST
		bindSingleton<Server>();
#endif
	}

	void configureBindings() const override {
#ifdef DI_SAUCE
		bind<PoiProvider>().in<di::SingletonScope>().to<PoiProvider(World&, TimeProvider&)>();
		bind<ServerLoop>().in<di::SingletonScope>().to<ServerLoop(Network&, SpawnMgr&, World&, EntityStorage&, EventBus&, AIRegistry&, ContainerProvider&, PoiProvider&)>();
		bind<AILoader>().in<di::SingletonScope>().to<AILoader(AIRegistry&)>();
		bind<EntityStorage>().in<di::SingletonScope>().to<EntityStorage(MessageSender&, World&, TimeProvider&, ContainerProvider&, PoiProvider&)>();
		bind<SpawnMgr>().in<di::SingletonScope>().to<SpawnMgr(World&, EntityStorage&, MessageSender&, TimeProvider&, AILoader&, ContainerProvider&, PoiProvider&)>();
#endif
#ifdef DI_BOOST
		bindSingleton<PoiProvider>();
		bindSingleton<ServerLoop>();
		bindSingleton<AILoader>();
		bindSingleton<EntityStorage>();
		bindSingleton<SpawnMgr>();
#endif
		bindSingleton<AIRegistry>();
		bindSingleton<ContainerProvider>();
		bindSingleton<World>();
	}
};
