/**
 * @file
 */

#pragma once

#include "ui/turbobadger/UIApp.h"
#include "RenderShaders.h"
#include "voxelfrontend/OctreeRenderer.h"
#include "frontend/ClientEntity.h"
#include "frontend/Movement.h"
#include "render/Axis.h"
#include "video/Camera.h"
#include "video/MeshPool.h"
#include "video/VertexBuffer.h"
#include "voxel/WorldMgr.h"

/**
 * @brief This tool will render the world as a client would, but with options to modify it.
 *
 * @ingroup Tools
 */
class ShapeTool: public ui::turbobadger::UIApp {
protected:
	using Super = ui::turbobadger::UIApp;
	video::Camera _camera;
	video::MeshPoolPtr _meshPool;
	voxelfrontend::OctreeRenderer _octreeRenderer;
	voxel::WorldPager _pager;
	voxel::PagedVolume *_volumeData = nullptr;
	voxel::BiomeManager _biomeManager;
	voxel::WorldContext _ctx;
	render::Axis _axis;
	core::VarPtr _speed;
	core::VarPtr _rotationSpeed;
	frontend::ClientEntityPtr _entity;
	video::ProfilerGPU _worldTimer = {"World"};
	ProfilerCPU _octreeTimer = {"Octree"};
	ProfilerCPU _frameTimer = {"Frame"};
	ProfilerCPU _beforeUiTimer = {"BeforeUI"};
	frontend::Movement _movement;

	bool _lineModeRendering = false;
	int _activeNodes = 0;

	bool onKeyPress(int32_t key, int16_t modifier) override;
	void onWindowResize() override;
	void beforeUI() override;
	void afterRootWidget() override;

public:
	ShapeTool(const metric::MetricPtr& metric, const video::MeshPoolPtr& meshPool, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
	~ShapeTool();

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
};
