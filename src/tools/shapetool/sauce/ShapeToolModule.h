#pragma once

#include <sauce/sauce.h>

#include "core/AbstractModule.h"
#include "core/App.h"
#include "core/App.h"

class ShapeToolModule: public core::AbstractModule {
	void configure() const override {
		core::AbstractModule::configure();
		bind<ShapeTool>().in<sauce::SingletonScope>().to<ShapeTool(io::Filesystem &, core::EventBus &, voxel::World &)>();
		bind<voxel::World>().in<sauce::SingletonScope>().to<voxel::World>();
	}
};
