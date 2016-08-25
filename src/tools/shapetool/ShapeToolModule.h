/**
 * @file
 */

#pragma once

#include "core/AppModule.h"
#include "video/MeshPool.h"

class ShapeToolModule: public core::AbstractAppModule {
	void configureApp() const override {
		bind<ShapeTool>().in<sauce::SingletonScope>().to<ShapeTool(video::MeshPool &, io::Filesystem &, core::EventBus &, voxel::World &)>();
	}

	void configureBindings() const override {
		bindSingleton<video::MeshPool>();
		bindSingleton<voxel::World>();
	}
};
