#pragma once

#include <string>
#include <inttypes.h>

namespace io {

class IEventObserver {
public:
	virtual ~IEventObserver() {
	}

	/* Prepare your app to go into the background. Stop loops, etc. This gets called when the user hits the home button, or gets a call. */
	virtual void onPrepareBackground() {
	}

	/* You will get this when your app is paused and iOS wants more memory. Release as much memory as possible. */
	virtual void onLowMemory() {
	}

	/* Terminate the app. Shut everything down before returning from this function. */
	virtual void onPrepareShutdown() {
	}

	/* This call happens when your app is coming back to the foreground. Restore all your state here. */
	virtual void onPrepareForeground() {
	}

	/* This will get called if the user accepted whatever sent your app to the background.
	 * If the user got a phone call and canceled it, you'll instead get an SDL_APP_DIDENTERFOREGROUND event and restart your loops.
	 * When you get this, you have 5 seconds to save all your state or the app will be terminated.
	 * Your app is NOT active at this point. */
	virtual void onBackground() {
	}

	/* Restart your loops here. Your app is interactive and getting CPU again. */
	virtual void onForeground() {
	}

	/**
	 * @param[in] horizontal
	 * @param[in] value The relative value that the joystick was moved, [-32768,32767]
	 */
	virtual void onJoystickMotion(bool horizontal, int value) {
	}

	virtual void onMouseWheel(int32_t x, int32_t y) {
	}

	virtual void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	}

	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button) {
	}

	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	}

	virtual void onJoystickDeviceRemoved(int32_t device) {
	}

	virtual void onJoystickDeviceAdded(int32_t device) {
	}

	virtual void onControllerButtonPress(const std::string& button) {
	}

	virtual void onControllerButtonRelease(const std::string& button) {
	}

	virtual void onJoystickButtonPress(uint8_t button) {
	}

	virtual void onJoystickButtonRelease(uint8_t button) {
	}

	virtual bool onTextInput(const std::string& text) {
		return false;
	}

	// returns true if the key release was handled
	virtual bool onKeyRelease(int32_t key) {
		return false;
	}

	// returns true if the key was handled
	virtual bool onKeyPress(int32_t key, int16_t modifier) {
		return false;
	}

	virtual bool onFingerPress(int64_t finger, float x, float y) {
		return false;
	}

	virtual bool onFingerRelease(int64_t finger, float x, float y) {
		return false;
	}

	virtual void onFingerMotion(int64_t finger, float x, float y, float dx, float dy) {
	}

	virtual void onWindowResize() {
	}

	virtual void onMultiGesture(float theta, float dist, int32_t numFingers) {
	}

	virtual void onGesture(int64_t gestureId, float error, int32_t numFingers) {
	}

	virtual void onGestureRecord(int64_t gestureId) {
	}
};

}
