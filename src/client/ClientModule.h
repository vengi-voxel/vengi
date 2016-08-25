/**
 * @file
 */

#pragma once

#include "core/AppModule.h"
#include "voxel/World.h"
#include "video/MeshPool.h"
#include "network/Network.h"
#include "network/MessageSender.h"

class ClientModule: public core::AbstractAppModule {
	virtual void configureApp() const override {
		bind<Client>().in<di::SingletonScope>().to<Client(video::MeshPool &, network::Network &, voxel::World &, network::MessageSender &, core::EventBus &, core::TimeProvider &, io::Filesystem &)>();
	}

	virtual void configureBindings() const override {
		bindSingleton<video::MeshPool>();
		bindSingleton<voxel::World>();
	}
};
