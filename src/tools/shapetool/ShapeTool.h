/**
 * @file
 */

#pragma once

#include "ui/UIApp.h"
#include "frontend/WorldShader.h"
#include "frontend/MeshShader.h"
#include "frontend/ColorShader.h"
#include "frontend/WorldRenderer.h"
#include "frontend/ClientEntity.h"
#include "video/Camera.h"
#include "video/MeshPool.h"
#include "video/VertexBuffer.h"
#include "voxel/World.h"

class ShapeTool: public ui::UIApp {
protected:
	voxel::WorldContext _ctx;
	video::Camera _camera;
	video::MeshPoolPtr _meshPool;
	frontend::WorldRenderer _worldRenderer;
	voxel::WorldPtr _world;
	frontend::WorldShader _worldShader;
	frontend::MeshShaderPtr _meshShader;
	frontend::ColorShaderPtr _colorShader;
	video::VertexBuffer _axisBuffer;
	core::VarPtr _speed;

	bool _resetTriggered = false;
	uint8_t _moveMask = 0;
	int _drawCallsWorld = 0;
	int _drawCallsEntities = 0;

	void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
	void beforeUI() override;
	void afterUI() override;

public:
	ShapeTool(video::MeshPoolPtr meshPool, io::FilesystemPtr filesystem, core::EventBusPtr eventBus, voxel::WorldPtr world);
	~ShapeTool();

	void reset(const voxel::WorldContext& ctx);
	void placeTree(const voxel::TreeContext& ctx);
	void regenerate(const glm::ivec2& pos);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
