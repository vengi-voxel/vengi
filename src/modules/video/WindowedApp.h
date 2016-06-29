/**
 * @file
 */

#pragma once

#include "core/App.h"
#include "io/IEventObserver.h"
#include "io/EventHandler.h"
#include "core/GLM.h"
#include "core/KeybindingParser.h"

struct SDL_Window;
typedef void *SDL_GLContext;

namespace video {

class WindowedApp: public core::App, public io::IEventObserver {
protected:
	SDL_Window* _window;
	SDL_GLContext _glcontext;
	int _width;
	int _height;
	float _aspect;

	typedef std::unordered_map<int32_t, int16_t> KeyMap;
	typedef KeyMap::const_iterator KeyMapConstIter;
	typedef KeyMap::iterator KeyMapIter;
	KeyMap _keys;
	core::BindMap _bindings;

	WindowedApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, uint16_t traceport = 17815);

	bool loadKeyBindings();

	virtual ~WindowedApp() {
	}
public:
	inline int width() const {
		return _width;
	}

	inline int height() const {
		return _height;
	}

	virtual core::AppState onRunning() override;
	virtual void onAfterRunning() override;
	virtual bool onKeyRelease(int32_t key) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
	virtual void onWindowResize() override;
};

}
