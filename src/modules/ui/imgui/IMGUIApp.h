/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/Camera.h"
#include "video/VertexBuffer.h"
#include "ImguiShaders.h"

namespace ui {
namespace imgui {

/**
 * @ingroup UI
 */
class IMGUIApp: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
protected:
	core::VarPtr _renderUI;
	video::Id _texture = video::InvalidId;
	shader::TextureShader _shader;
	video::Camera _camera;
	video::VertexBuffer _vbo;
	int32_t _bufferIndex = -1;
	int32_t _indexBufferIndex = -1;
	int8_t _mouseWheel = 0;
	bool _mousePressed[3];

	virtual bool onKeyRelease(int32_t key) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const std::string& text) override;
	virtual void onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
public:
	IMGUIApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport = 17815);
	virtual ~IMGUIApp();

	virtual void beforeUI() {
	}

	virtual void onWindowResize() override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual void onRenderUI() = 0;
	virtual core::AppState onCleanup() override;
};

}
}
