/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "render/BloomRenderer.h"
#include "render/BlurRenderer.h"
#include "video/Texture.h"

/**
 * https://learnopengl.com/Advanced-Lighting/Bloom
 */
class TestBloom: public TestApp {
private:
	using Super = TestApp;
	render::BloomRenderer _bloomRenderer;
	render::BlurRenderer _blurRenderer;
	video::TexturePtr _sceneTexture;
	video::TexturePtr _bloomTexture;

	int _passes = 10;
	bool _bloom = true;

	void doRender() override;
public:
	TestBloom(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual app::AppState onInit() override;
	virtual app::AppState onCleanup() override;
	virtual void onRenderUI() override;
};
