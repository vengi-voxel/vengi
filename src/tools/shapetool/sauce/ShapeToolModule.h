#pragma once

#include <sauce/sauce.h>

#include "core/AbstractModule.h"
#include "video/MeshPool.h"
#include "core/App.h"

class ShapeToolModule: public core::AbstractModule {
	void configure() const override {
		core::AbstractModule::configure();
		bind<ShapeTool>().in<sauce::SingletonScope>().to<ShapeTool(video::MeshPool &, io::Filesystem &, core::EventBus &, voxel::World &)>();
		bind<video::MeshPool>().in<sauce::SingletonScope>().to<video::MeshPool>();
		bind<voxel::World>().in<sauce::SingletonScope>().to<voxel::World>();
	}
};
