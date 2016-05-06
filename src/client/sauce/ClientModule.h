/**
 * @file
 */

#pragma once

#include <sauce/sauce.h>

#include "core/AbstractModule.h"
#include "voxel/World.h"
#include "video/MeshPool.h"
#include "network/Network.h"
#include "network/MessageSender.h"

class ClientModule: public core::AbstractModule {
	void configure() const override {
		core::AbstractModule::configure();
		bind<Client>().in<sauce::SingletonScope>().to<Client(video::MeshPool &, network::Network &, voxel::World &, network::MessageSender &, core::EventBus &, core::TimeProvider &, io::Filesystem &)>();
		bind<video::MeshPool>().in<sauce::SingletonScope>().to<video::MeshPool>();
		bind<voxel::World>().in<sauce::SingletonScope>().to<voxel::World>();
	}
};
