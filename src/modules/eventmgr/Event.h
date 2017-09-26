/**
 * @file
 */

#pragma once

#include "EventId.h"
#include <memory>

namespace eventmgr {

class Event {
private:
	EventId _id;
public:
	Event(EventId id);

	bool start();
	bool stop();

	bool update(long dt);
};

typedef std::shared_ptr<Event> EventPtr;

}
