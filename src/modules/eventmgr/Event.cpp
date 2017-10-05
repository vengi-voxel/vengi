/**
 * @file
 */

#include "Event.h"

namespace eventmgr {

Event::Event(EventId id) :
		_id(id) {
}

Event::~Event() {
}

bool Event::start() {
	return true;
}

bool Event::stop() {
	return true;
}

bool Event::update(long dt) {
	return true;
}

void Event::shutdown() {
}

}
