/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/Camera.h"
#include "video/Buffer.h"
#include "RenderShaders.h"
#include "Console.h"
#include <atomic>
#include <map>
#include <stack>
#include <mutex>
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
	bool _renderTracing = false;
	std::mutex _traceMutex;
	static constexpr int _maxMeasureSize = 200;
	struct TraceData {
		TraceData(uint64_t _value, const char *_name);
		~TraceData();
		const uint64_t value;
		const char * const name;
		uint64_t delta = 0ul;
		std::vector<TraceData*> children;
	};
	class TraceRoot {
	private:
		std::stack<TraceData*> queue;
	public:
		TraceRoot(uint64_t nanos, const char *name);
		TraceRoot(TraceRoot&&);
		~TraceRoot();
		/**
		 * @brief Insert entry into the current active node
		 * @param[in] nanos The nano second timestamp of the measurement
		 * @param[in] name The name of the measurement
		 */
		void begin(uint64_t nanos, const char *name);
		/**
		 * @brief Calculate the delta of the last matching begin block
		 * @param[in] nanos The nano second timestamp of end of the measurement
		 */
		void end(uint64_t nanos);
		TraceData *data;
	};
	using Measures = std::map<std::thread::id, TraceRoot*>;
	int _currentFrameCounter = 0;
	Measures _traceMeasures;
	Measures _traceMeasuresLastFrame[_maxMeasureSize];
	bool _traceMeasuresPause = false;
	using FramesMillis = core::Array<uint64_t, _maxMeasureSize>;
	FramesMillis _frameMillis {{0ul}};
	core::String _writePathIni;
	core::String _writePathLog;

	virtual void traceBeginFrame(const char *threadName) override;
	virtual void traceBegin(const char *threadName, const char* name) override;
	virtual void traceEnd(const char *threadName) override;
	virtual void traceEndFrame(const char *threadName) override;
	void addSubTrees(const TraceData* traceData, bool expandAll, int &depth) const;
	void renderTracing();

	virtual bool onKeyRelease(int32_t key, int16_t modifier) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const core::String& text) override;
	virtual bool onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
public:
	IMGUIApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
	virtual ~IMGUIApp();

	virtual void beforeUI() {
	}

	virtual void onWindowResize(int windowWidth, int windowHeight) override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual void onAfterFrame() override;
	virtual void onRenderUI() = 0;
	virtual core::AppState onCleanup() override;
};

}
}
