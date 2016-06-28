/**
 * @file
 */

#pragma once

#include <sauce/sauce.h>

#include "core/AbstractModule.h"
#include "video/MeshPool.h"
#include "core/App.h"

class ShapeToolModule: public core::AbstractAppModule {
	void configureApp() const override {
		bind<ShapeTool>().in<sauce::SingletonScope>().to<ShapeTool(video::MeshPool &, io::Filesystem &, core::EventBus &, voxel::World &)>();
	}

	void configureBindings() const override {
		bindSingleton<video::MeshPool>();
		bindSingleton<voxel::World>();
	}
};
