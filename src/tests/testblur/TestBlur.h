/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "render/BlurRenderer.h"
#include "video/Texture.h"

/**
 * https://learnopengl.com/Advanced-Lighting/Bloom
 */
class TestBlur: public TestApp {
private:
	using Super = TestApp;
	render::BlurRenderer _blurRenderer;
	video::TexturePtr _sceneTexture;

	int _passes = 10;

	void doRender() override;
public:
	TestBlur(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual app::AppState onInit() override;
	virtual app::AppState onCleanup() override;
	virtual void onRenderUI() override;
};
