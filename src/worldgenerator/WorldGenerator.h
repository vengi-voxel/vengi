#pragma once

#include "core/App.h"
#include "voxel/World.h"
#include "core/TimeProvider.h"

class WorldGenerator: public core::App {
private:
	voxel::WorldPtr _world;
	core::TimeProviderPtr _timeProvider;
	long _seed;
	int _size;
public:
	WorldGenerator(voxel::WorldPtr world, core::EventBusPtr eventBus, core::TimeProviderPtr timeProvider, io::FilesystemPtr filesystem);

	core::AppState onInit() override;
	core::AppState onRunning() override;
};
