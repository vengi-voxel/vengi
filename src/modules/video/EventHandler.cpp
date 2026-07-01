/**
 * @file
 */

#include "EventHandler.h"
#include "core/Log.h"
#include "video/IEventObserver.h"
#include "core/GLM.h"
#include "core/Assert.h"
#include <glm/common.hpp>
#include <SDL3/SDL_version.h>

#include <SDL3/SDL_events.h>

namespace video {

EventHandler::EventHandler() {
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

static PenAxis mapPenAxis(SDL_PenAxis axis) {
	switch (axis) {
	case SDL_PEN_AXIS_PRESSURE:
		return PenAxis::Pressure;
	case SDL_PEN_AXIS_XTILT:
		return PenAxis::Xtilt;
	case SDL_PEN_AXIS_YTILT:
		return PenAxis::Ytilt;
	case SDL_PEN_AXIS_DISTANCE:
		return PenAxis::Distance;
	case SDL_PEN_AXIS_ROTATION:
		return PenAxis::Rotation;
	case SDL_PEN_AXIS_SLIDER:
		return PenAxis::Slider;
	case SDL_PEN_AXIS_TANGENTIAL_PRESSURE:
		return PenAxis::TangentialPressure;
	default:
		break;
	}
	return PenAxis::Unknown;
}

core::String EventHandler::getControllerButtonName(uint8_t button) {
	const char *name = SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));
	if (name == nullptr) {
		return "unknown";
	}
	return name;
}

bool EventHandler::handleEvent(SDL_Event &event) {
	switch (event.type) {
	case SDL_DROPFILE: {
		core::String data = event.drop.data;
		dropFile((void*)SDL_GetWindowFromID(event.drop.windowID), data);
		break;
	}
	case SDL_DROPTEXT: {
		core::String data = event.drop.data;
		dropText((void*)SDL_GetWindowFromID(event.drop.windowID), data);
		break;
	}
	case SDL_TEXTINPUT:
		textInput((void*)SDL_GetWindowFromID(event.text.windowID), core::String(event.text.text));
		break;
	case SDL_KEYUP: {
		void* handle = (void*)SDL_GetWindowFromID(event.key.windowID);
		SDL_Keycode key = event.key.key;
		SDL_Keymod mod = event.key.mod;
		keyRelease(handle, (int32_t)key, (int16_t)mod);
		break;
	}
	case SDL_KEYDOWN: {
		void* handle = (void*)SDL_GetWindowFromID(event.key.windowID);
		SDL_Keycode key = event.key.key;
		SDL_Keymod mod = event.key.mod;
		keyPress(handle, (int32_t)key, (int16_t)mod);
		break;
	}
	case SDL_MOUSEMOTION: {
		SDL_Window *window = SDL_GetWindowFromID(event.motion.windowID);
		mouseMotion((void*)window, event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel, event.motion.which);
		break;
	}
	case SDL_MOUSEBUTTONDOWN:
		mouseButtonPress((void*)SDL_GetWindowFromID(event.button.windowID), event.button.x, event.button.y, event.button.button, event.button.clicks, event.button.which);
		break;
	case SDL_MOUSEBUTTONUP:
		mouseButtonRelease((void*)SDL_GetWindowFromID(event.button.windowID), event.button.x, event.button.y, event.button.button, event.button.which);
		break;
	case SDL_MOUSEWHEEL: {
		float x;
		float y;
		x = event.wheel.x;
		y = event.wheel.y;
		x = glm::clamp(x, -1.0f, 1.0f);
		y = glm::clamp(y, -1.0f, 1.0f);
		mouseWheel((void*)SDL_GetWindowFromID(event.wheel.windowID), x, y, event.wheel.which);
		break;
	}
	case SDL_CONTROLLERAXISMOTION: {
		const uint8_t axis = event.gaxis.axis;
		const int value = event.gaxis.value;
		const uint32_t id = event.gaxis.which;
		controllerMotion(axis, value, id);
		break;
	}
	case SDL_CONTROLLERBUTTONDOWN: {
		uint8_t button = event.gbutton.button;
		uint32_t id = event.gbutton.which;
		controllerButtonPress(getControllerButtonName(button), id);
		break;
	}
	case SDL_CONTROLLERBUTTONUP: {
		uint8_t button = event.gbutton.button;
		uint32_t id = event.gbutton.which;
		controllerButtonRelease(getControllerButtonName(button), id);
		break;
	}
	case SDL_CONTROLLERDEVICEADDED: {
		controllerDeviceAdded(event.cdevice.which);
		break;
	}
	case SDL_CONTROLLERDEVICEREMOVED: {
		controllerDeviceRemoved(event.cdevice.which);
		break;
	}
	case SDL_JOYHATMOTION:
	case SDL_JOYDEVICEADDED:
	case SDL_JOYDEVICEREMOVED:
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
	case SDL_JOYAXISMOTION:
		// ignore joystick events - use gamecontroller events
		break;
	case SDL_FINGERDOWN: {
		SDL_Window *window = SDL_GetWindowFromID(event.tfinger.windowID);
		int64_t finger = event.tfinger.fingerID;
		fingerPress((void*)window, finger, event.tfinger.x, event.tfinger.y, event.tfinger.pressure, event.tfinger.timestamp);
		break;
	}
	case SDL_FINGERUP: {
		SDL_Window *window = SDL_GetWindowFromID(event.tfinger.windowID);
		int64_t finger = event.tfinger.fingerID;
		fingerRelease((void*)window, finger, event.tfinger.x, event.tfinger.y, event.tfinger.timestamp);
		break;
	}
	case SDL_FINGERMOTION: {
		SDL_Window *window = SDL_GetWindowFromID(event.tfinger.windowID);
		int64_t finger = event.tfinger.fingerID;
		fingerMotion((void*)window, finger, event.tfinger.x, event.tfinger.y, event.tfinger.dx, event.tfinger.dy, event.tfinger.pressure, event.tfinger.timestamp);
		break;
	}
	case SDL_WINDOWEVENT_RESTORED: {
		SDL_Window* window = SDL_GetWindowFromID(event.window.windowID);
		for (IEventObserver* observer : _observers) {
			observer->onWindowRestore((void*)window);
		}
		break;
	}
	case SDL_WINDOWEVENT_RESIZED: {
		SDL_Window* window = SDL_GetWindowFromID(event.window.windowID);
		for (IEventObserver* observer : _observers) {
			const int w = event.window.data1;
			const int h = event.window.data2;
			observer->onWindowResize((void*)window, w, h);
		}
		break;
	}
	case SDL_WINDOWEVENT_CLOSE: {
		SDL_Window* window = SDL_GetWindowFromID(event.window.windowID);
		for (IEventObserver* observer : _observers) {
			observer->onWindowClose((void*)window);
		}
		break;
	}
	case SDL_WINDOWEVENT_MOVED: {
		SDL_Window* window = SDL_GetWindowFromID(event.window.windowID);
		for (IEventObserver* observer : _observers) {
			observer->onWindowMoved((void*)window);
		}
		break;
	}
	case SDL_WINDOWEVENT_FOCUS_GAINED: {
		SDL_Window* window = SDL_GetWindowFromID(event.window.windowID);
		for (IEventObserver* observer : _observers) {
			observer->onWindowFocusGained((void*)window);
		}
		break;
	}
	case SDL_WINDOWEVENT_FOCUS_LOST: {
		SDL_Window* window = SDL_GetWindowFromID(event.window.windowID);
		for (IEventObserver* observer : _observers) {
			observer->onWindowFocusLost((void*)window);
		}
		break;
	}
	case SDL_EVENT_PEN_AXIS: {
		SDL_Window* window = SDL_GetWindowFromID(event.paxis.windowID);
		PenAxis axis = mapPenAxis(event.paxis.axis);
		for (IEventObserver* observer : _observers) {
			observer->onPenAxis((void *)window, event.paxis.which, event.paxis.x, event.paxis.y, axis, event.paxis.value);
		}
		break;
	}
	case SDL_EVENT_PEN_DOWN: {
		SDL_Window *window = SDL_GetWindowFromID(event.ptouch.windowID);
		for (IEventObserver *observer : _observers) {
			observer->onPenDown((void *)window, event.ptouch.which, event.ptouch.x, event.ptouch.y, event.ptouch.eraser);
		}
		break;
	}
	case SDL_EVENT_PEN_UP: {
		SDL_Window *window = SDL_GetWindowFromID(event.ptouch.windowID);
		for (IEventObserver *observer : _observers) {
			observer->onPenUp((void *)window, event.ptouch.which, event.ptouch.x, event.ptouch.y, event.ptouch.eraser);
		}
		break;
	}
	case SDL_EVENT_PEN_BUTTON_DOWN: {
		SDL_Window* window = SDL_GetWindowFromID(event.pbutton.windowID);
		for (IEventObserver* observer : _observers) {
			observer->onPenButtonDown((void*)window, event.pbutton.which, event.pbutton.x, event.pbutton.y, event.pbutton.button);
		}
		break;
	}
	case SDL_EVENT_PEN_BUTTON_UP: {
		SDL_Window* window = SDL_GetWindowFromID(event.pbutton.windowID);
		for (IEventObserver* observer : _observers) {
			observer->onPenButtonUp((void*)window, event.pbutton.which, event.pbutton.x, event.pbutton.y, event.pbutton.button);
		}
		break;
	}
	case SDL_EVENT_PEN_PROXIMITY_IN: {
		SDL_Window* window = SDL_GetWindowFromID(event.pproximity.windowID);
		for (IEventObserver* observer : _observers) {
			observer->onPenProximityIn((void*)window, event.pproximity.which);
		}
		break;
	}
	case SDL_EVENT_PEN_PROXIMITY_OUT: {
		SDL_Window* window = SDL_GetWindowFromID(event.pproximity.windowID);
		for (IEventObserver* observer : _observers) {
			observer->onPenProximityOut((void*)window, event.pproximity.which);
		}
		break;
	}
	case SDL_EVENT_PEN_MOTION: {
		SDL_Window *window = SDL_GetWindowFromID(event.pmotion.windowID);
		for (IEventObserver *observer : _observers) {
			observer->onPenMotion((void *)window, event.pmotion.which, event.pmotion.x,
								  event.pmotion.y);
		}
		break;
	}
	default:
		break;
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
	default:
		break;
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

void EventHandler::mouseWheel(void *windowHandle, float x, float y, int32_t mouseId) {
	Log::trace("Mouse wheel: x=%f y=%f (%d)", x, y, mouseId);
	for (IEventObserver* observer : _observers) {
		observer->onMouseWheel(windowHandle, x, y, mouseId);
	}
}

void EventHandler::mouseMotion(void *windowHandle, int32_t x, int32_t y, int32_t relX, int32_t relY, int32_t mouseId) {
	Log::trace("Mouse motion: x=%d y=%d relX=%d relY=%d (%d)", x, y, relX, relY, mouseId);
	for (IEventObserver* observer : _observers) {
		observer->onMouseMotion(windowHandle, x, y, relX, relY, mouseId);
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
	Log::trace("Controller axis motion: axis %u value %d (%u)", axis, value, id);
	for (IEventObserver* observer : _observers) {
		observer->onControllerMotion(axis, value, id);
	}
}

void EventHandler::controllerButtonPress(const core::String& button, uint32_t id) {
	Log::trace("Controller button pressed: %s (%u)", button.c_str(), id);
	for (IEventObserver* observer : _observers) {
		observer->onControllerButtonPress(button, id);
	}
}

void EventHandler::controllerButtonRelease(const core::String& button, uint32_t id) {
	Log::trace("Controller button released: %s (%u)", button.c_str(), id);
	for (IEventObserver* observer : _observers) {
		observer->onControllerButtonRelease(button, id);
	}
}

void EventHandler::mouseButtonPress(void *windowHandle, int32_t x, int32_t y, uint8_t button, uint8_t clicks, int32_t mouseId) {
	Log::trace("Mouse button %d pressed at %d:%d with %d clicks", button, x, y, clicks);
	for (IEventObserver* observer : _observers) {
		observer->onMouseButtonPress(windowHandle, x, y, button, clicks, mouseId);
	}
}

void EventHandler::mouseButtonRelease(void *windowHandle, int32_t x, int32_t y, uint8_t button, int32_t mouseId) {
	Log::trace("Mouse button %d released at %d:%d", button, x, y);
	for (IEventObserver* observer : _observers) {
		observer->onMouseButtonRelease(windowHandle, x, y, button, mouseId);
	}
}

void EventHandler::dropFile(void *windowHandle, const core::String& file) {
	Log::trace("File dropped: %s", file.c_str());
	for (IEventObserver* observer : _observers) {
		observer->onDropFile(windowHandle, file);
	}
}

void EventHandler::dropText(void *windowHandle, const core::String& text) {
	Log::trace("Text dropped: %s", text.c_str());
	for (IEventObserver* observer : _observers) {
		observer->onDropText(windowHandle, text);
	}
}

void EventHandler::textInput(void *windowHandle, const core::String& text) {
	Log::trace("Text input: %s", text.c_str());
	for (IEventObserver* observer : _observers) {
		observer->onTextInput(windowHandle, text);
	}
}

void EventHandler::keyRelease(void *windowHandle, int32_t key, int16_t modifier) {
	Log::trace("Key released: %d", key);
	for (IEventObserver* observer : _observers) {
		observer->onKeyRelease(windowHandle, key, modifier);
	}
}

void EventHandler::keyPress(void *windowHandle, int32_t key, int16_t modifier) {
	Log::trace("Key pressed: %d", key);
	for (IEventObserver* observer : _observers) {
		observer->onKeyPress(windowHandle, key, modifier);
	}
}

void EventHandler::fingerPress(void *windowHandle, int64_t finger, float x, float y, float pressure, uint32_t timestamp) {
	for (IEventObserver* observer : _observers) {
		observer->onFingerPress(windowHandle, finger, x, y, pressure, timestamp);
	}
}

void EventHandler::fingerRelease(void *windowHandle, int64_t finger, float x, float y, uint32_t timestamp) {
	for (IEventObserver* observer : _observers) {
		observer->onFingerRelease(windowHandle, finger, x, y, timestamp);
	}
}

void EventHandler::fingerMotion(void *windowHandle, int64_t finger, float x, float y, float dx, float dy, float pressure, uint32_t timestamp) {
	for (IEventObserver* observer : _observers) {
		observer->onFingerMotion(windowHandle, finger, x, y, dx, dy, pressure, timestamp);
	}
}

}
