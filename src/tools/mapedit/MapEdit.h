/**
 * @file
 */

#pragma once

#include "ui/imgui/IMGUIApp.h"
#include "FrontendShaders.h"
#include "frontend/WorldRenderer.h"
#include "frontend/ClientEntity.h"
#include "frontend/Axis.h"
#include "video/Camera.h"
#include "video/MeshPool.h"
#include "video/VertexBuffer.h"
#include "voxel/World.h"

/**
 * @brief This is the map editor to place entities with procgen settings to form a map in the world
 *
 * @note Can also be used to 'just' render a map.
 *
 * @ingroup Tools
 */
class MapEdit: public ui::imgui::IMGUIApp {
protected:
	using Super = ui::imgui::IMGUIApp;
	video::Camera _camera;
	video::MeshPoolPtr _meshPool;
	frontend::WorldRenderer _worldRenderer;
	voxel::WorldPtr _world;
	frontend::Axis _axis;
	core::VarPtr _speed;
	core::VarPtr _rotationSpeed;
	frontend::ClientEntityPtr _entity;
	video::ProfilerGPU _worldTimer = {"World"};
	ProfilerCPU _frameTimer = {"Frame"};
	ProfilerCPU _beforeUiTimer = {"BeforeUI"};

	bool _lineModeRendering = false;
	uint8_t _moveMask = 0;
	bool _freelook = false;
	bool _updateWorld = true;
	int _drawCallsWorld = 0;
	int _vertices = 0;
	int _drawCallsEntities = 0;

	void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
	void onWindowResize() override;
	void beforeUI() override;

public:
	MapEdit(const metric::MetricPtr& metric, const video::MeshPoolPtr& meshPool, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const voxel::WorldPtr& world);
	~MapEdit();

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	void onRenderUI() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
};
