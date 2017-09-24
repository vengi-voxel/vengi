/**
 * @file
 */

#include "Event.h"

namespace eventmgr {

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
