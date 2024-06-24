/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <inttypes.h>

namespace video {

/**
 * @brief Register implementations at the EventHandler class to get notifications on the events
 */
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

	virtual bool onMouseWheel(float x, float y) {
		return false;
	}

	virtual void onMouseMotion(void *windowHandle, int32_t x, int32_t y, int32_t relX, int32_t relY) {
	}

	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	}

	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	}

	/**
	 * @param[in] axis SDL_GameControllerAxis
	 * @param[in] value -32768 to 32767
	 * @param[in] id game controller id
	 */
	virtual void onControllerMotion(uint8_t axis, int value, uint32_t id) {
	}

	virtual void onControllerDeviceRemoved(int32_t device) {
	}

	virtual void onControllerDeviceAdded(int32_t device) {
	}

	virtual void onControllerButtonPress(const core::String& button, uint32_t id) {
	}

	virtual void onControllerButtonRelease(const core::String& button, uint32_t id) {
	}

	virtual void onDropFile(const core::String& file) {
	}

	virtual void onDropText(const core::String& text) {
	}

	virtual bool onTextInput(const core::String& text) {
		return false;
	}

	// returns true if the key release was handled
	virtual bool onKeyRelease(int32_t key, int16_t modifier) {
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

	virtual void onWindowResize(void *windowHandle, int windowWidth, int windowHeight) {
	}

	virtual void onWindowClose(void *windowHandle) {
	}

	virtual void onWindowMoved(void *windowHandle) {
	}

	virtual void onWindowFocusGained(void *windowHandle) {
	}

	virtual void onWindowFocusLost(void *windowHandle) {
	}

	virtual void onWindowRestore(void *windowHandle) {
	}

	/**
	 * @param[in] theta the amount that the fingers rotated during this motion.
	 * @param[in] dist the amount that the fingers pinched during this motion.
	 * @param[in] numFingers the number of fingers used in the gesture.
	 */
	virtual void onMultiGesture(float theta, float dist, int32_t numFingers) {
	}

	/**
	 * @brief Callback for dollar gestures
	 * @note Can be recorded by pushing the record window onto the ui stack
	 * @param[in] gestureId the unique id of the closest gesture to the performed stroke.
	 * @param[in] error the difference between the gesture template and the actual performed gesture. Lower error is a better match.
	 * @param[in] numFingers the number of fingers used to draw the stroke.
	 */
	virtual void onGesture(int64_t gestureId, float error, int32_t numFingers) {
	}

	virtual void onGestureRecord(int64_t gestureId) {
	}
};

}
