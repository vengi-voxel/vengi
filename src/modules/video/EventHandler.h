/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include <stdint.h>

union SDL_Event;

namespace video {

class IEventObserver;

/**
 * @brief Register IEventObserver implementations to spread the events in the system
 */
class EventHandler {
private:
	bool _multiGesture;

	typedef core::DynamicArray<IEventObserver*> EventObservers;
	EventObservers _observers;
	struct Event {
		Event(IEventObserver* _observer, bool _remove) : observer(_observer), remove(_remove) {
		}

		IEventObserver* observer;
		bool remove;
	};
	core::DynamicArray<Event> _events;

	static core::String getControllerButtonName(uint8_t button);

public:
	EventHandler();
	virtual ~EventHandler();

	// returns true if the processed event did not lead to the application quit
	bool handleEvent(SDL_Event &event);
	// returns true if the event was processed, false if it should get added to the event queue.
	bool handleAppEvent(SDL_Event &event);

	void registerObserver(IEventObserver* observer);
	void removeObserver(IEventObserver* observer);
	void controllerDeviceAdded(int32_t device);
	void controllerDeviceRemoved(int32_t device);
	void lowMemory();
	void prepareShutdown();
	void prepareBackground();
	void prepareForeground();
	void background();
	void foreground();
	void controllerButtonPress(const core::String& button, uint32_t id);
	void controllerButtonRelease(const core::String& button, uint32_t id);
	/**
	 * @param[in] axis SDL_GameControllerAxis
	 * @param[in] value -32768 to 32767
	 * @param[in] id game controller id
	 */
	void controllerMotion(uint8_t axis, int value, uint32_t id);
	void mouseWheel(float x, float y);
	void mouseMotion(void *windowHandle, int32_t x, int32_t y, int32_t relX, int32_t relY);
	void mouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks);
	void mouseButtonRelease(int32_t x, int32_t y, uint8_t button);
	void dropFile(const core::String& file);
	void dropText(const core::String& text);
	void textInput(const core::String& text);
	void keyRelease(int32_t key, int16_t modifier = 0);
	void keyPress(int32_t key, int16_t modifier = 0);
	void fingerPress(int64_t finger, float x, float y);
	void fingerRelease(int64_t finger, float x, float y);
	void fingerMotion(int64_t finger, float x, float y, float dx, float dy);
	void gestureRecord(int64_t gestureId);
	void gesture(int64_t gestureId, float error, int32_t numFingers);
	/**
	 * @param[in] theta the amount that the fingers rotated during this motion.
	 * @param[in] dist the amount that the fingers pinched during this motion.
	 * @param[in] numFingers the number of fingers used in the gesture.
	 */
	void multiGesture(float theta, float dist, int32_t numFingers);
};

}
