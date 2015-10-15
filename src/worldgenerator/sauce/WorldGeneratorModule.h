#pragma once

#include "core/AbstractModule.h"
#include "voxel/World.h"

class WorldGeneratorModule: public core::AbstractModule {
	void configure() const override {
		core::AbstractModule::configure();
		bind<WorldGenerator>().in<sauce::SingletonScope>().to<WorldGenerator(voxel::World &, core::EventBus &, core::TimeProvider &, io::Filesystem &)>();
		bind<voxel::World>().in<sauce::SingletonScope>().to<voxel::World>();
	}
};
