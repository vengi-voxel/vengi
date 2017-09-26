/**
 * @file
 */

#include "Event.h"

namespace eventmgr {

Event::Event(EventId id) :
		_id(id) {
}

bool Event::stop() {
	return false;
}

bool Event::start() {
	return false;
}

bool Event::update(long dt) {
	return true;
}

}
