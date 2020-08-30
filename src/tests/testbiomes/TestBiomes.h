/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "voxelworld/BiomeManager.h"
#include "video/Texture.h"
#include "render/TextureRenderer.h"
#include "core/collection/ConcurrentQueue.h"
#include "EventResult.h"

class TestBiomes: public TestApp {
private:
	using Super = TestApp;

	core::ConcurrentQueue<Event*> _workQueue;
	core::ConcurrentQueue<Result*> _resultQueue;

	voxelworld::BiomeManager _biomeMgr;
	glm::ivec3 _biomesPos;
	glm::ivec2 _biomesTextureSize { 512, 512 };

	video::TexturePtr _texture;
	render::TextureRenderer _renderer;

	void doRender() override;
	void onRenderUI() override;

	void recalcBiomes(const glm::ivec3& pos);
public:
	TestBiomes(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual app::AppState onInit() override;
	virtual app::AppState onRunning() override;
	virtual app::AppState onCleanup() override;
};
