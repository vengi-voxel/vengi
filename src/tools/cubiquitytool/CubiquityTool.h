#pragma once

#include "frontend/TerrainShader.h"
#include "frontend/ColoredCubesShader.h"
#include "ui/UIApp.h"
#include "frontend/WorldRenderer.h"
#include "video/Camera.h"
#include "voxel/World.h"
#include "video/GLFunc.h"

class CubiquityTool: public ui::UIApp {
protected:
	voxel::World::WorldContext _ctx;
	video::Camera _camera;
	frontend::WorldRenderer _worldRenderer;
	voxel::WorldPtr _world;
	frontend::TerrainShader _terrainShader;
	frontend::ColoredCubesShader _coloredCubesShader;
	video::Shader* _currentShader;

	uint8_t _moveMask = 0;

	void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	void beforeUI() override;
public:
	CubiquityTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus, voxel::WorldPtr world);
	~CubiquityTool();

	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
