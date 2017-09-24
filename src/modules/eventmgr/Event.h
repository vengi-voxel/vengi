/**
 * @file
 */

#pragma once

#include <memory>

namespace eventmgr {

using EventId = int;

class Event {
private:
	EventId id;
public:
	bool start();
	bool stop();

	bool update(long dt);
};

typedef std::shared_ptr<Event> EventPtr;

}
