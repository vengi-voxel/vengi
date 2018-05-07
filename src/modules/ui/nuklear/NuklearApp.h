/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/Camera.h"
#include "video/Buffer.h"
#include "video/Texture.h"
#include "Console.h"
#include "RenderShaders.h"
#include "Nuklear.h"

namespace ui {
namespace nuklear {

/**
 * @ingroup UI
 */
class NuklearApp: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
public:
	struct Vertex {
		float x, y;
		float u, v;
		union {
			struct { uint8_t r, g, b, a; };
			uint32_t col;
		};
	};
protected:
	struct nk_context _ctx;
	struct nk_font_atlas _atlas;
	struct nk_draw_null_texture _null;
	struct nk_buffer _cmds;
	struct nk_convert_config _config;
	struct nk_font *_font = nullptr;

	Console _console;

	shader::TextureShader _shader;
	video::Camera _camera;
	video::Buffer _vbo;
	video::TexturePtr _fontTexture;

	int32_t _vertexBufferIndex = -1;
	int32_t _elementBufferIndex = -1;

	bool onKeyEvent(int32_t sym, int16_t modifier, bool down);

	virtual bool onKeyRelease(int32_t key) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const std::string& text) override;
	virtual void onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;
public:
	NuklearApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
	virtual ~NuklearApp();

	virtual bool onRenderUI() = 0;

	virtual void onWindowResize() override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
};

}
}
