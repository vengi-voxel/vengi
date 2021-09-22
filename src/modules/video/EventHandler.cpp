/**
 * @file
 */

#include "EventHandler.h"
#include "video/IEventObserver.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "core/Assert.h"
#include <glm/common.hpp>
#include <SDL.h>

namespace io {

EventHandler::EventHandler() :
		_multiGesture(false) {
	//SDL_JoystickEventState(SDL_DISABLE);
	SDL_EventState(SDL_JOYAXISMOTION, SDL_ENABLE);
}

EventHandler::~EventHandler() {
}

void EventHandler::registerObserver(IEventObserver* observer) {
	core_assert(observer != nullptr);
	_events.emplace_back(observer, false);
}

void EventHandler::removeObserver(IEventObserver* observer) {
	core_assert(observer != nullptr);
	_events.emplace_back(observer, true);
}

core::String EventHandler::getControllerButtonName(uint8_t button) {
	const char *name = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(button));
	if (name == nullptr) {
		return "unknown";
	}
	return name;
}

bool EventHandler::handleEvent(SDL_Event &event) {
	switch (event.type) {
	case SDL_DROPFILE:
		dropFile(event.drop.file);
		break;
	case SDL_TEXTINPUT:
		textInput(core::String(event.text.text));
		break;
	case SDL_KEYUP:
		keyRelease((int32_t) event.key.keysym.sym, (int16_t) event.key.keysym.mod);
		break;
	case SDL_KEYDOWN:
		keyPress((int32_t) event.key.keysym.sym, (int16_t) event.key.keysym.mod);
		break;
	case SDL_MOUSEMOTION: {
		if (event.motion.which == SDL_TOUCH_MOUSEID) {
			break;
		}
		SDL_Window *window = SDL_GetWindowFromID(event.motion.windowID);
		if ((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) == 0u) {
			break;
		}
		mouseMotion(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
		break;
	}
	case SDL_MOUSEBUTTONDOWN:
		if (event.button.which == SDL_TOUCH_MOUSEID) {
			break;
		}
		mouseButtonPress(event.button.x, event.button.y, event.button.button, event.button.clicks);
		break;
	case SDL_MOUSEBUTTONUP:
		if (event.button.which == SDL_TOUCH_MOUSEID) {
			break;
		}
		mouseButtonRelease(event.button.x, event.button.y, event.button.button);
		break;
	case SDL_MOUSEWHEEL: {
		if (event.wheel.which == SDL_TOUCH_MOUSEID) {
			break;
		}
		int x = event.wheel.x;
		int y = event.wheel.y;
		if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
			x *= -1;
			y *= -1;
		}
		x = glm::clamp(x, -1, 1);
		y = glm::clamp(y, -1, 1);
		mouseWheel(x, y);
		break;
	}
	case SDL_CONTROLLERAXISMOTION:
		controllerMotion(event.caxis.axis, event.caxis.value, event.caxis.which);
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		controllerButtonPress(getControllerButtonName(event.cbutton.button), event.cbutton.which);
		break;
	case SDL_CONTROLLERBUTTONUP:
		controllerButtonRelease(getControllerButtonName(event.cbutton.button), event.cbutton.which);
		break;
	case SDL_CONTROLLERDEVICEADDED:
		controllerDeviceAdded(event.cdevice.which);
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
		controllerDeviceRemoved(event.cdevice.which);
		break;
	case SDL_DOLLARRECORD:
		gestureRecord(event.dgesture.gestureId);
		break;
	case SDL_DOLLARGESTURE:
		gesture(event.dgesture.gestureId, event.dgesture.error, event.dgesture.numFingers);
		break;
	case SDL_MULTIGESTURE:
		multiGesture(event.mgesture.dTheta, event.mgesture.dDist, event.mgesture.numFingers);
		break;
	case SDL_JOYHATMOTION:
	case SDL_JOYDEVICEADDED:
	case SDL_JOYDEVICEREMOVED:
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
	case SDL_JOYAXISMOTION:
		// ignore joystick events - use gamecontroller events
		break;
	case SDL_FINGERDOWN:
		fingerPress(event.tfinger.fingerId, event.tfinger.x, event.tfinger.y);
		break;
	case SDL_FINGERUP:
		fingerRelease(event.tfinger.fingerId, event.tfinger.x, event.tfinger.y);
		break;
	case SDL_FINGERMOTION:
		fingerMotion(event.tfinger.fingerId, event.tfinger.x, event.tfinger.y, event.tfinger.dx, event.tfinger.dy);
		break;
	case SDL_WINDOWEVENT: {
		SDL_Window* window = SDL_GetWindowFromID(event.window.windowID);
		switch (event.window.event) {
		case SDL_WINDOWEVENT_RESTORED:
			for (IEventObserver* observer : _observers) {
				observer->onWindowRestore((void*)window);
			}
			break;
		case SDL_WINDOWEVENT_RESIZED:
			for (IEventObserver* observer : _observers) {
				const int w = event.window.data1;
				const int h = event.window.data2;
				observer->onWindowResize((void*)window, w, h);
			}
			break;
		case SDL_WINDOWEVENT_CLOSE:
			for (IEventObserver* observer : _observers) {
				observer->onWindowClose((void*)window);
			}
			return false;
		case SDL_WINDOWEVENT_MOVED:
			for (IEventObserver* observer : _observers) {
				observer->onWindowMoved((void*)window);
			}
			return false;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			for (IEventObserver* observer : _observers) {
				observer->onWindowFocusGained((void*)window);
			}
			return false;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			for (IEventObserver* observer : _observers) {
				observer->onWindowFocusLost((void*)window);
			}
			return false;
		}
		break;
	}
	}

	// Traverse through the list and try to find the specified observer
	for (const Event& obsEvent : _events) {
		if (obsEvent.remove) {
			for (EventObservers::iterator i = _observers.begin(); i != _observers.end(); ++i) {
				if (*i == obsEvent.observer) {
					_observers.erase(i);
					break;
				}
			}
		} else {
			_observers.push_back(obsEvent.observer);
		}
	}
	_events.clear();

	return true;
}

bool EventHandler::handleAppEvent(SDL_Event &event) {
	switch (event.type) {
	case SDL_APP_TERMINATING:
		prepareShutdown();
		break;
	case SDL_APP_LOWMEMORY:
		lowMemory();
		break;
	case SDL_APP_WILLENTERBACKGROUND:
		prepareBackground();
		return true;
	case SDL_APP_DIDENTERBACKGROUND:
		background();
		return true;
	case SDL_APP_WILLENTERFOREGROUND:
		prepareForeground();
		return true;
	case SDL_APP_DIDENTERFOREGROUND:
		foreground();
		return true;
	}
	return false;
}

void EventHandler::lowMemory() {
	for (IEventObserver* observer : _observers) {
		observer->onLowMemory();
	}
}

void EventHandler::prepareShutdown() {
	for (IEventObserver* observer : _observers) {
		observer->onPrepareShutdown();
	}
}

void EventHandler::prepareBackground() {
	for (IEventObserver* observer : _observers) {
		observer->onPrepareBackground();
	}
}

void EventHandler::prepareForeground() {
	for (IEventObserver* observer : _observers) {
		observer->onPrepareForeground();
	}
}

void EventHandler::background() {
	for (IEventObserver* observer : _observers) {
		observer->onBackground();
	}
}

void EventHandler::foreground() {
	for (IEventObserver* observer : _observers) {
		observer->onForeground();
	}
}

void EventHandler::mouseWheel(int32_t x, int32_t y) {
	for (IEventObserver* observer : _observers) {
		observer->onMouseWheel(x, y);
	}
}

void EventHandler::mouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	for (IEventObserver* observer : _observers) {
		observer->onMouseMotion(x, y, relX, relY);
	}
}

void EventHandler::controllerDeviceAdded(int32_t device) {
	for (IEventObserver* observer : _observers) {
		observer->onControllerDeviceAdded(device);
	}
}

void EventHandler::controllerDeviceRemoved(int32_t device) {
	for (IEventObserver* observer : _observers) {
		observer->onControllerDeviceRemoved(device);
	}
}

void EventHandler::controllerMotion(uint8_t axis, int value, uint32_t id) {
	for (IEventObserver* observer : _observers) {
		observer->onControllerMotion(axis, value, id);
	}
}

void EventHandler::controllerButtonPress(const core::String& button, uint32_t id) {
	for (IEventObserver* observer : _observers) {
		observer->onControllerButtonPress(button, id);
	}
}

void EventHandler::controllerButtonRelease(const core::String& button, uint32_t id) {
	for (IEventObserver* observer : _observers) {
		observer->onControllerButtonRelease(button, id);
	}
}

void EventHandler::mouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	for (IEventObserver* observer : _observers) {
		observer->onMouseButtonPress(x, y, button, clicks);
	}
}

void EventHandler::mouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	for (IEventObserver* observer : _observers) {
		observer->onMouseButtonRelease(x, y, button);
	}
}

void EventHandler::dropFile(const core::String& file) {
	for (IEventObserver* observer : _observers) {
		observer->onDropFile(file);
	}
}

void EventHandler::textInput(const core::String& text) {
	for (IEventObserver* observer : _observers) {
		observer->onTextInput(text);
	}
}

void EventHandler::keyRelease(int32_t key, int16_t modifier) {
	for (IEventObserver* observer : _observers) {
		observer->onKeyRelease(key, modifier);
	}
}

void EventHandler::keyPress(int32_t key, int16_t modifier) {
	for (IEventObserver* observer : _observers) {
		observer->onKeyPress(key, modifier);
	}
}

void EventHandler::fingerPress(int64_t finger, float x, float y) {
	for (IEventObserver* observer : _observers) {
		observer->onFingerPress(finger, x, y);
	}
}

void EventHandler::fingerRelease(int64_t finger, float x, float y) {
	_multiGesture = false;
	for (IEventObserver* observer : _observers) {
		observer->onFingerRelease(finger, x, y);
	}
}

void EventHandler::fingerMotion(int64_t finger, float x, float y, float dx, float dy) {
	if (_multiGesture) {
		return;
	}
	for (IEventObserver* observer : _observers) {
		observer->onFingerMotion(finger, x, y, dx, dy);
	}
}

void EventHandler::gestureRecord(int64_t gestureId) {
	for (IEventObserver* observer : _observers) {
		observer->onGestureRecord(gestureId);
	}
}

void EventHandler::gesture(int64_t gestureId, float error, int32_t numFingers) {
	for (IEventObserver* observer : _observers) {
		observer->onGesture(gestureId, error, numFingers);
	}
}

void EventHandler::multiGesture(float theta, float dist, int32_t numFingers) {
	_multiGesture = true;
	for (IEventObserver* observer : _observers) {
		observer->onMultiGesture(theta, dist, numFingers);
	}
}

}
