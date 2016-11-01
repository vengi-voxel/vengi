/**
 * @file
 */

#pragma once

#include <SDL.h>
#include <unordered_map>
#include <list>
#include <tb_widgets_listener.h>

#include "video/WindowedApp.h"
#include "Window.h"
#include "Console.h"

namespace ui {

class UIApp: public video::WindowedApp, private tb::TBWidgetListener {
private:
	using Super = video::WindowedApp;
protected:
	/**
	 * @brief This struct allows you to determine how long a key was pressed or how long it is pressed
	 */
	struct KeyState {
		// how long was the key down
		long downtime = 0;
		// when was the key pressed
		long msec = 0;
		// is it still pressed
		bool active = false;
	};

	/**
	 * @brief Call this in a key up key binding function
	 */
	inline void keyUp(KeyState& state) {
		state.active = false;
		state.downtime = _now - state.msec;
		state.downtime = std::max(_now - state.msec, 10L);
	}

	/**
	 * @brief Call this in a key down key binding function
	 */
	inline void keyDown(KeyState& state) {
		if (state.active) {
			return;
		}
		state.msec = _now;
		state.active = true;
		state.downtime = 0;
	}

	bool _quit;
	tb::TBWidget _root;
	int fps = 0;
	uint32_t _frameCounter = 0;
	double _frameCounterResetRime = 0.0;
	Console _console;
	core::VarPtr _renderUI;
	int _lastShowTextY = -1;

	virtual bool onKeyRelease(int32_t key) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const std::string& text) override;
	virtual void onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;

	virtual void OnWidgetFocusChanged(tb::TBWidget *widget, bool focused) override;

	bool invokeKey(int key, tb::SPECIAL_KEY special, tb::MODIFIER_KEYS mod, bool down);
	void showStr(int x, int y, const glm::vec4& color, const char *fmt, ...) __attribute__((format(printf, 5, 6)));
	void enqueueShowStr(int x, const glm::vec4& color, const char *fmt, ...) __attribute__((format(printf, 4, 5)));

	tb::MODIFIER_KEYS getModifierKeys() const;
public:
	UIApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport = 17815);
	virtual ~UIApp();

	virtual void beforeUI() {
	}

	template<class T>
	T* getWidgetByType(const char *name);

	tb::TBWidget* getWidget(const char *name);
	tb::TBWidget* getWidgetAt(int x, int y, bool includeChildren = true);

	// hook that is called directory before the ui is rendered. Your last chance to let the app contribute
	// something in the ui context and the ui drawcalls (like debug text or rendering an in-game console on
	// top of the ui)
	virtual void afterRootWidget();

	void addChild(Window* window);

	void doLayout();

	virtual void onWindowResize() override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
};

template<class T>
inline T* UIApp::getWidgetByType(const char *name) {
	return _root.GetWidgetByIDAndType<T>(tb::TBID(name));
}

}
