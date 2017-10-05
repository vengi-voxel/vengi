/**
 * @file
 */

#pragma once

#include "EventId.h"
#include <memory>

namespace eventmgr {

class Event {
protected:
	EventId _id;
public:
	Event(EventId id);
	virtual ~Event();

	/**
	 * @brief Event start method if the start time is met
	 * @return @c true if the event was started successfully, @c false otherwise.
	 */
	bool start();
	/**
	 * @brief Stop method for the event if the end time of the event configuration is met.
	 * @return @c true if the event was stopped successfully, @c false otherwise.
	 */
	bool stop();
	/**
	 * @brief Shutdown method that is called when the end time of the event is not yet
	 * reached, but the @c EventMgr is shut down.
	 */
	void shutdown();
	/**
	 * @brief Update the event and all related components.
	 * @param[in] dt The delta time since the last update was called (milliseconds)
	 * @return @c true if the event was ticked successfully.
	 */
	virtual bool update(long dt);
};

typedef std::shared_ptr<Event> EventPtr;

}
