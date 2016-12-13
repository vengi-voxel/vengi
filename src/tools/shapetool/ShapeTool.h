/**
 * @file
 */

#pragma once

#include "ui/UIApp.h"
#include "FrontendShaders.h"
#include "frontend/WorldRenderer.h"
#include "frontend/ClientEntity.h"
#include "frontend/Axis.h"
#include "video/Camera.h"
#include "video/MeshPool.h"
#include "video/VertexBuffer.h"
#include "voxel/World.h"

/**
 * @brief This tool will render the world as a client would, but with options to modify it.
 */
class ShapeTool: public ui::UIApp {
protected:
	using Super = ui::UIApp;
	voxel::WorldContext _ctx;
	video::Camera _camera;
	video::MeshPoolPtr _meshPool;
	frontend::WorldRenderer _worldRenderer;
	voxel::WorldPtr _world;
	frontend::Axis _axis;
	core::VarPtr _speed;
	core::VarPtr _rotationSpeed;
	frontend::ClientEntityPtr _entity;
	ProfilerGPU _worldTimer;
	ProfilerCPU _frameTimer;
	ProfilerCPU _beforeUiTimer;

	bool _resetTriggered = false;
	bool _lineModeRendering = false;
	uint8_t _moveMask = 0;
	bool _freelook = false;
	int _drawCallsWorld = 0;
	int _vertices = 0;
	int _drawCallsEntities = 0;

	void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
	void onWindowResize() override;
	void beforeUI() override;
	void afterRootWidget() override;

public:
	ShapeTool(const video::MeshPoolPtr& meshPool, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const voxel::WorldPtr& world);
	~ShapeTool();

	void reset(const voxel::WorldContext& ctx);
	void regenerate(const glm::ivec2& pos);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
};
