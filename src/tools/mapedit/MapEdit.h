/**
 * @file
 */

#pragma once

#include "ui/imgui/IMGUIApp.h"
#include "RenderShaders.h"
#include "voxelrender/WorldRenderer.h"
#include "frontend/ClientEntity.h"
#include "render/Axis.h"
#include "frontend/Movement.h"
#include "video/Camera.h"
#include "video/MeshPool.h"
#include "video/VertexBuffer.h"
#include "voxel/WorldMgr.h"

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
	voxelrender::WorldRenderer _worldRenderer;
	voxel::WorldMgrPtr _worldMgr;
	render::Axis _axis;
	core::VarPtr _speed;
	core::VarPtr _rotationSpeed;
	frontend::ClientEntityPtr _entity;
	video::ProfilerGPU _worldTimer = {"World"};
	ProfilerCPU _frameTimer = {"Frame"};
	ProfilerCPU _beforeUiTimer = {"BeforeUI"};
	frontend::Movement _movement;

	bool _lineModeRendering = false;
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
	MapEdit(const metric::MetricPtr& metric, const video::MeshPoolPtr& meshPool, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const voxel::WorldMgrPtr& world);
	~MapEdit();

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	void onRenderUI() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
};
