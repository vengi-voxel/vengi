#pragma once

#include "ui/UIApp.h"
#include "frontend/WorldShader.h"
#include "frontend/WorldRenderer.h"
#include "video/Camera.h"
#include "voxel/World.h"

class ShapeTool: public ui::UIApp {
protected:
	voxel::WorldContext _ctx;
	video::Camera _camera;
	frontend::WorldRenderer _worldRenderer;
	voxel::WorldPtr _world;
	frontend::WorldShader _worldShader;

	bool _resetTriggered = false;
	uint8_t _moveMask = 0;

	void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
	void beforeUI() override;

public:
	ShapeTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus, voxel::WorldPtr world);
	~ShapeTool();

	void reset(const voxel::WorldContext& ctx);
	void placeTree(const voxel::TreeContext& ctx);
	void regenerate(const glm::ivec2& pos);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
