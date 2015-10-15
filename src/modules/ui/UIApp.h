#pragma once

#include <SDL.h>
#include <unordered_map>
#include <list>

#include "video/WindowedApp.h"
#include "KeybindingParser.h"
#include "Window.h"

namespace ui {

class UIApp: public video::WindowedApp {
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
		if (state.active)
			return;
		state.msec = _now;
		state.active = true;
		state.downtime = 0;
	}

	typedef std::unordered_map<int32_t, int16_t> KeyMap;
	typedef KeyMap::const_iterator KeyMapConstIter;
	typedef KeyMap::iterator KeyMapIter;
	KeyMap _keys;
	BindMap _bindings;
	bool _quit;
	tb::TBWidget _root;
	int fps = 0;
	uint32_t _frameCounter = 0;
	double _frameCounterResetRime = 0.0;

	bool loadKeyBindings();

	virtual bool onKeyRelease(int32_t key) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const std::string& text) override;
	virtual void onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;

	bool invokeKey(int key, tb::SPECIAL_KEY special, tb::MODIFIER_KEYS mod, bool down);
public:
	UIApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus);
	virtual ~UIApp();

	virtual void beforeUI() {
	}

	void addChild(Window* window);

	virtual void afterUI();

	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
};

}
