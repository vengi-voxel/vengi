/**
 * @file
 */

#include "Event.h"

namespace eventmgr {

Event::Event(EventId id, const EventConfigurationDataPtr& data) :
		_id(id), _type(data->type) {
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
