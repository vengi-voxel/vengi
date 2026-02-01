/**
 * @file
 */

#include "EventHandler.h"
#include "core/Log.h"
#include "video/IEventObserver.h"
#include "core/GLM.h"
#include "core/Assert.h"
#include <glm/common.hpp>
#include <SDL_version.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
#include <SDL_events.h>
#define SDL_APP_DIDENTERBACKGROUND SDL_EVENT_DID_ENTER_BACKGROUND
#define SDL_APP_DIDENTERFOREGROUND SDL_EVENT_DID_ENTER_FOREGROUND
#define SDL_APP_LOWMEMORY SDL_EVENT_LOW_MEMORY
#define SDL_APP_TERMINATING SDL_EVENT_TERMINATING
#define SDL_APP_WILLENTERBACKGROUND SDL_EVENT_WILL_ENTER_BACKGROUND
#define SDL_APP_WILLENTERFOREGROUND SDL_EVENT_WILL_ENTER_FOREGROUND
#define SDL_AUDIODEVICEADDED SDL_EVENT_AUDIO_DEVICE_ADDED
#define SDL_AUDIODEVICEREMOVED SDL_EVENT_AUDIO_DEVICE_REMOVED
#define SDL_CLIPBOARDUPDATE SDL_EVENT_CLIPBOARD_UPDATE
#define SDL_CONTROLLERAXISMOTION SDL_EVENT_GAMEPAD_AXIS_MOTION
#define SDL_CONTROLLERBUTTONDOWN SDL_EVENT_GAMEPAD_BUTTON_DOWN
#define SDL_CONTROLLERBUTTONUP SDL_EVENT_GAMEPAD_BUTTON_UP
#define SDL_CONTROLLERDEVICEADDED SDL_EVENT_GAMEPAD_ADDED
#define SDL_CONTROLLERDEVICEREMAPPED SDL_EVENT_GAMEPAD_REMAPPED
#define SDL_CONTROLLERDEVICEREMOVED SDL_EVENT_GAMEPAD_REMOVED
#define SDL_CONTROLLERSENSORUPDATE SDL_EVENT_GAMEPAD_SENSOR_UPDATE
#define SDL_CONTROLLERSTEAMHANDLEUPDATED SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED
#define SDL_CONTROLLERTOUCHPADDOWN SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN
#define SDL_CONTROLLERTOUCHPADMOTION SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION
#define SDL_CONTROLLERTOUCHPADUP SDL_EVENT_GAMEPAD_TOUCHPAD_UP
#define SDL_ControllerAxisEvent SDL_GamepadAxisEvent
#define SDL_ControllerButtonEvent SDL_GamepadButtonEvent
#define SDL_ControllerDeviceEvent SDL_GamepadDeviceEvent
#define SDL_ControllerSensorEvent SDL_GamepadSensorEvent
#define SDL_ControllerTouchpadEvent SDL_GamepadTouchpadEvent
#define SDL_DISPLAYEVENT_CONNECTED SDL_EVENT_DISPLAY_ADDED
#define SDL_DISPLAYEVENT_DISCONNECTED SDL_EVENT_DISPLAY_REMOVED
#define SDL_DISPLAYEVENT_MOVED SDL_EVENT_DISPLAY_MOVED
#define SDL_DISPLAYEVENT_ORIENTATION SDL_EVENT_DISPLAY_ORIENTATION
#define SDL_DROPBEGIN SDL_EVENT_DROP_BEGIN
#define SDL_DROPCOMPLETE SDL_EVENT_DROP_COMPLETE
#define SDL_DROPFILE SDL_EVENT_DROP_FILE
#define SDL_DROPTEXT SDL_EVENT_DROP_TEXT
#define SDL_DelEventWatch SDL_RemoveEventWatch
#define SDL_FINGERDOWN SDL_EVENT_FINGER_DOWN
#define SDL_FINGERMOTION SDL_EVENT_FINGER_MOTION
#define SDL_FINGERUP SDL_EVENT_FINGER_UP
#define SDL_FIRSTEVENT SDL_EVENT_FIRST
#define SDL_JOYAXISMOTION SDL_EVENT_JOYSTICK_AXIS_MOTION
#define SDL_JOYBATTERYUPDATED SDL_EVENT_JOYSTICK_BATTERY_UPDATED
#define SDL_JOYBUTTONDOWN SDL_EVENT_JOYSTICK_BUTTON_DOWN
#define SDL_JOYBUTTONUP SDL_EVENT_JOYSTICK_BUTTON_UP
#define SDL_JOYDEVICEADDED SDL_EVENT_JOYSTICK_ADDED
#define SDL_JOYDEVICEREMOVED SDL_EVENT_JOYSTICK_REMOVED
#define SDL_JOYBALLMOTION SDL_EVENT_JOYSTICK_BALL_MOTION
#define SDL_JOYHATMOTION SDL_EVENT_JOYSTICK_HAT_MOTION
#define SDL_KEYDOWN SDL_EVENT_KEY_DOWN
#define SDL_KEYMAPCHANGED SDL_EVENT_KEYMAP_CHANGED
#define SDL_KEYUP SDL_EVENT_KEY_UP
#define SDL_LASTEVENT SDL_EVENT_LAST
#define SDL_LOCALECHANGED SDL_EVENT_LOCALE_CHANGED
#define SDL_MOUSEBUTTONDOWN SDL_EVENT_MOUSE_BUTTON_DOWN
#define SDL_MOUSEBUTTONUP SDL_EVENT_MOUSE_BUTTON_UP
#define SDL_MOUSEMOTION SDL_EVENT_MOUSE_MOTION
#define SDL_MOUSEWHEEL SDL_EVENT_MOUSE_WHEEL
#define SDL_POLLSENTINEL SDL_EVENT_POLL_SENTINEL
#define SDL_QUIT SDL_EVENT_QUIT
#define SDL_RENDER_DEVICE_RESET SDL_EVENT_RENDER_DEVICE_RESET
#define SDL_RENDER_TARGETS_RESET SDL_EVENT_RENDER_TARGETS_RESET
#define SDL_SENSORUPDATE SDL_EVENT_SENSOR_UPDATE
#define SDL_TEXTEDITING SDL_EVENT_TEXT_EDITING
#define SDL_TEXTEDITING_EXT SDL_EVENT_TEXT_EDITING_EXT
#define SDL_TEXTINPUT SDL_EVENT_TEXT_INPUT
#define SDL_USEREVENT SDL_EVENT_USER
#define SDL_WINDOWEVENT_CLOSE SDL_EVENT_WINDOW_CLOSE_REQUESTED
#define SDL_WINDOWEVENT_DISPLAY_CHANGED SDL_EVENT_WINDOW_DISPLAY_CHANGED
#define SDL_WINDOWEVENT_ENTER SDL_EVENT_WINDOW_MOUSE_ENTER
#define SDL_WINDOWEVENT_EXPOSED SDL_EVENT_WINDOW_EXPOSED
#define SDL_WINDOWEVENT_FOCUS_GAINED SDL_EVENT_WINDOW_FOCUS_GAINED
#define SDL_WINDOWEVENT_FOCUS_LOST SDL_EVENT_WINDOW_FOCUS_LOST
#define SDL_WINDOWEVENT_HIDDEN SDL_EVENT_WINDOW_HIDDEN
#define SDL_WINDOWEVENT_HIT_TEST SDL_EVENT_WINDOW_HIT_TEST
#define SDL_WINDOWEVENT_ICCPROF_CHANGED SDL_EVENT_WINDOW_ICCPROF_CHANGED
#define SDL_WINDOWEVENT_LEAVE SDL_EVENT_WINDOW_MOUSE_LEAVE
#define SDL_WINDOWEVENT_MAXIMIZED SDL_EVENT_WINDOW_MAXIMIZED
#define SDL_WINDOWEVENT_MINIMIZED SDL_EVENT_WINDOW_MINIMIZED
#define SDL_WINDOWEVENT_MOVED SDL_EVENT_WINDOW_MOVED
#define SDL_WINDOWEVENT_RESIZED SDL_EVENT_WINDOW_RESIZED
#define SDL_WINDOWEVENT_RESTORED SDL_EVENT_WINDOW_RESTORED
#define SDL_WINDOWEVENT_SHOWN SDL_EVENT_WINDOW_SHOWN
#define SDL_WINDOWEVENT_SIZE_CHANGED SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED
#else
#include "core/sdl/SDLSystem.h"
#endif

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

#if SDL_VERSION_ATLEAST(3, 2, 0)
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
#endif

core::String EventHandler::getControllerButtonName(uint8_t button) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	const char *name = SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));
#else
	const char *name = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(button));
#endif
	if (name == nullptr) {
		return "unknown";
	}
	return name;
}

bool EventHandler::handleEvent(SDL_Event &event) {
	switch (event.type) {
	case SDL_DROPFILE: {
#if SDL_VERSION_ATLEAST(3, 2, 0)
		core::String data = event.drop.data;
#else
		core::String data = event.drop.file;
		SDL_free(event.drop.file);
#endif
		dropFile((void*)SDL_GetWindowFromID(event.drop.windowID), data);
		break;
	}
	case SDL_DROPTEXT: {
#if SDL_VERSION_ATLEAST(3, 2, 0)
		core::String data = event.drop.data;
#else
		core::String data = event.drop.file;
		SDL_free(event.drop.file);
#endif
		dropText((void*)SDL_GetWindowFromID(event.drop.windowID), data);
		break;
	}
	case SDL_TEXTINPUT:
		textInput((void*)SDL_GetWindowFromID(event.text.windowID), core::String(event.text.text));
		break;
	case SDL_KEYUP: {
		void* handle = (void*)SDL_GetWindowFromID(event.key.windowID);
#if SDL_VERSION_ATLEAST(3, 2, 0)
		SDL_Keycode key = event.key.key;
		SDL_Keymod mod = event.key.mod;
#else
		SDL_Keycode key = event.key.keysym.sym;
		Uint16 mod = event.key.keysym.mod;
#endif
		keyRelease(handle, (int32_t)key, (int16_t)mod);
		break;
	}
	case SDL_KEYDOWN: {
		void* handle = (void*)SDL_GetWindowFromID(event.key.windowID);
#if SDL_VERSION_ATLEAST(3, 2, 0)
		SDL_Keycode key = event.key.key;
		SDL_Keymod mod = event.key.mod;
#else
		SDL_Keycode key = event.key.keysym.sym;
		Uint16 mod = event.key.keysym.mod;
#endif
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
#if SDL_VERSION_ATLEAST(3, 2, 0)
		x = event.wheel.x;
		y = event.wheel.y;
#elif SDL_VERSION_ATLEAST(2, 0, 18)
		x = event.wheel.preciseX;
		y = event.wheel.preciseY;
#else
		x = (float)event.wheel.x;
		y = (float)event.wheel.y;
#endif
		x = glm::clamp(x, -1.0f, 1.0f);
		y = glm::clamp(y, -1.0f, 1.0f);
		mouseWheel((void*)SDL_GetWindowFromID(event.wheel.windowID), x, y, event.wheel.which);
		break;
	}
	case SDL_CONTROLLERAXISMOTION: {
#if SDL_VERSION_ATLEAST(3, 2, 0)
		const uint8_t axis = event.gaxis.axis;
		const int value = event.gaxis.value;
		const uint32_t id = event.gaxis.which;
#else
		const uint8_t axis = event.caxis.axis;
		const int value = event.caxis.value;
		const uint32_t id = event.caxis.which;
#endif
		controllerMotion(axis, value, id);
		break;
	}
	case SDL_CONTROLLERBUTTONDOWN: {
#if SDL_VERSION_ATLEAST(3, 2, 0)
		uint8_t button = event.gbutton.button;
		uint32_t id = event.gbutton.which;
#else
		uint8_t button = event.cbutton.button;
		uint32_t id = event.cbutton.which;
#endif
		controllerButtonPress(getControllerButtonName(button), id);
		break;
	}
	case SDL_CONTROLLERBUTTONUP: {
#if SDL_VERSION_ATLEAST(3, 2, 0)
		uint8_t button = event.gbutton.button;
		uint32_t id = event.gbutton.which;
#else
		uint8_t button = event.cbutton.button;
		uint32_t id = event.cbutton.which;
#endif
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
#if SDL_VERSION_ATLEAST(3, 2, 0)
		int64_t finger = event.tfinger.fingerID;
#else
		int64_t finger = event.tfinger.fingerId;
#endif
		fingerPress((void*)window, finger, event.tfinger.x, event.tfinger.y, event.tfinger.pressure, event.tfinger.timestamp);
		break;
	}
	case SDL_FINGERUP: {
		SDL_Window *window = SDL_GetWindowFromID(event.tfinger.windowID);
#if SDL_VERSION_ATLEAST(3, 2, 0)
		int64_t finger = event.tfinger.fingerID;
#else
		int64_t finger = event.tfinger.fingerId;
#endif
		fingerRelease((void*)window, finger, event.tfinger.x, event.tfinger.y, event.tfinger.timestamp);
		break;
	}
	case SDL_FINGERMOTION: {
		SDL_Window *window = SDL_GetWindowFromID(event.tfinger.windowID);
#if SDL_VERSION_ATLEAST(3, 2, 0)
		int64_t finger = event.tfinger.fingerID;
#else
		int64_t finger = event.tfinger.fingerId;
#endif
		fingerMotion((void*)window, finger, event.tfinger.x, event.tfinger.y, event.tfinger.dx, event.tfinger.dy, event.tfinger.pressure, event.tfinger.timestamp);
		break;
	}
#if !SDL_VERSION_ATLEAST(3, 2, 0)
	case SDL_WINDOWEVENT: {
		switch (event.window.event) {
#endif
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
#if !SDL_VERSION_ATLEAST(3, 2, 0)
		default:
			break;
		}
		break;
	}
#endif

#if SDL_VERSION_ATLEAST(3, 2, 0)
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
#endif
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
