/**
 * @file
 */

#pragma once

#include "core/App.h"
#include "io/IEventObserver.h"
#include "io/EventHandler.h"
#include "core/GLM.h"

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
	glm::vec4 _clearColor;

	WindowedApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, uint16_t traceport = 17815);

	virtual ~WindowedApp() {
	}
public:
	virtual core::AppState onRunning() override;
	virtual void onAfterRunning() override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
	virtual void onWindowResize() override;
};

}
