/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/Camera.h"
#include "video/VertexBuffer.h"
#include "ImguiShaders.h"
#include "Console.h"
#include <atomic>
#include <map>
#include <thread>
#include <deque>
#include <mutex>
#include <array>

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
	Console _console;
	int32_t _bufferIndex = -1;
	int32_t _indexBufferIndex = -1;
	int8_t _mouseWheel = 0;
	bool _mousePressed[3];
	bool _renderTracing = false;
	std::mutex _traceMutex;
	static constexpr int _maxMeasureSize = 200;
	struct TraceData {
		int cnt = 0;
		bool begin = true;
		double value = -1.0;
		double delta = 0.0;
	};
	using FrameData = std::map<const char*, TraceData>;
	using Frames = std::array<FrameData, _maxMeasureSize>;
	using Measures = std::map<std::thread::id, Frames>;
	thread_local static int _currentFrameCounter;
	Measures _traceMeasures;
	using FramesMillis = std::array<float, _maxMeasureSize>;
	FramesMillis _frameMillis {0.0f};

	virtual void traceBeginFrame(const char *threadName) override;
	virtual void traceBegin(const char *threadName, const char* name) override;
	virtual void traceEnd(const char *threadName) override;
	virtual void traceEndFrame(const char *threadName) override;
	void renderTracing();

	virtual bool onKeyRelease(int32_t key) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const std::string& text) override;
	virtual void onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
public:
	IMGUIApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
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
