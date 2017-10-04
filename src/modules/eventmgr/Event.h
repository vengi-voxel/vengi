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
	virtual ~Event();

	bool start();
	bool stop();

	virtual bool update(long dt);
};

typedef std::shared_ptr<Event> EventPtr;

}
