/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/Camera.h"
#include "video/Buffer.h"
#include "RenderShaders.h"
#include "Console.h"
#include <map>
#include <stack>
#include "core/collection/Array.h"

namespace ui {
namespace imgui {

// https://github.com/aiekick/ImGuiFileDialog
/**
 * @ingroup UI
 */
class IMGUIApp: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
	video::Camera _camera;
protected:
	core::VarPtr _renderUI;
	video::Id _texture = video::InvalidId;
	shader::TextureShader _shader;
	video::Buffer _vbo;
	Console _console;
	int32_t _bufferIndex = -1;
	int32_t _indexBufferIndex = -1;
	int8_t _mouseWheel = 0;
	bool _mousePressed[3] = {false};
	core::String _writePathIni;
	core::String _writePathLog;

	void executeDrawCommands();

	virtual bool onKeyRelease(int32_t key, int16_t modifier) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const core::String& text) override;
	virtual bool onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
public:
	IMGUIApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem,
			const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~IMGUIApp();

	virtual void beforeUI() {
	}

	virtual void onWindowResize(int windowWidth, int windowHeight) override;
	virtual app::AppState onConstruct() override;
	virtual app::AppState onInit() override;
	virtual app::AppState onRunning() override;
	virtual void onRenderUI() = 0;
	virtual app::AppState onCleanup() override;
};

}
}
