/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "testcore/TextureRenderer.h"
#include "video/Texture.h"
#include "PatternShader.h"

/**
 * @brief Visual test for compute shaders.
 *
 * A compute shader fills a texture (UV gradient plus a centered circle).
 * The texture is then drawn fullscreen so --screenshot can capture it.
 */
class TestComputeShader : public TestApp {
private:
	using Super = TestApp;
	shader::PatternShader _computeShader;
	render::TextureRenderer _renderer;
	video::TexturePtr _texture;
	static constexpr int TextureSize = 256;

	void doRender() override;

public:
	TestComputeShader(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onInit() override;
	app::AppState onCleanup() override;
};
