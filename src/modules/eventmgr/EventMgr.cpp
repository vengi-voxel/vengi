/**
 * @file
 */

#include "EventMgr.h"
#include "EventMgrModels.h"
#include "core/Log.h"

namespace eventmgr {

EventMgr::EventMgr() {
}

bool EventMgr::init() {
	if (!db::EventModel::createTable()) {
		Log::error("Failed to create event table");
		return false;
	}
	if (!db::EventPointModel::createTable()) {
		Log::error("Failed to create event point table");
		return false;
	}

	return true;
}

void EventMgr::update(long dt) {
	for (auto& e : _events) {
		if (!e.second->update(dt)) {
			e.second->stop();
			// TODO: remove event from collection
		}
	}
}

void EventMgr::shutdown() {
	for (auto& e : _events) {
		e.second->stop();
	}
	_events.clear();
}

bool EventMgr::startEvent(EventId id) {
	// TODO: load event data from lua scripts and add to _events
	return false;
}

bool EventMgr::stopEvent(EventId id) {
	auto i = _events.find(id);
	if (i == _events.end()) {
		return false;
	}
	i->second->stop();
	_events.erase(i);
	return true;
}

}
