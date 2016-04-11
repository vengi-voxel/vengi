#pragma once

#include <sauce/sauce.h>

#include "core/AbstractModule.h"
#include "core/App.h"

class CubiquityToolModule: public core::AbstractModule {
	void configure() const override {
		core::AbstractModule::configure();
		bind<CubiquityTool>().in<sauce::SingletonScope>().to<CubiquityTool(io::Filesystem &, core::EventBus &, voxel::World &)>();
		bind<voxel::World>().in<sauce::SingletonScope>().to<voxel::World>();
	}
};
