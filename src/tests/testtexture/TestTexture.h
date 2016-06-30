/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/Texture.h"
#include "TesttextureShaders.h"
#include "video/VertexBuffer.h"

class TestTexture: public TestApp {
private:
	using Super = TestApp;
	video::TexturePtr _texture;
	shader::TextureShader _textureShader;
	video::VertexBuffer _texturedFullscreenQuad;

	void doRender() override;
public:
	TestTexture(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
