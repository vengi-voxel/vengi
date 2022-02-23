/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "render/BloomRenderer.h"
#include "video/Texture.h"

/**
 * https://learnopengl.com/Advanced-Lighting/Bloom
 */
class TestBloom: public TestApp {
private:
	using Super = TestApp;
	render::BloomRenderer _bloomRenderer;
	video::TexturePtr _sceneTexture;
	video::TexturePtr _glowTexture;

	void doRender() override;
public:
	TestBloom(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual app::AppState onInit() override;
	virtual app::AppState onCleanup() override;
	virtual void onRenderUI() override;
};
