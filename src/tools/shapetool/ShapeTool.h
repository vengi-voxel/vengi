#pragma once

#include "ui/UIApp.h"
#include "frontend/WorldShader.h"
#include "frontend/WorldRenderer.h"
#include "video/Camera.h"

class ShapeTool: public ui::UIApp {
protected:
	video::Camera _camera;
	frontend::WorldRenderer _worldRenderer;
	voxel::WorldPtr _world;
	frontend::WorldShader _worldShader;

	bool _resetTriggered = false;

	void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	bool onKeyPress(int32_t key, int16_t modifier) override;

public:
	ShapeTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus, voxel::WorldPtr world);
	~ShapeTool();

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
};
