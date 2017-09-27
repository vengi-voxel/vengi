/**
 * @file
 */

#include "EventMgr.h"
#include "EventMgrModels.h"
#include "core/Log.h"

namespace eventmgr {

EventMgr::EventMgr(const persistence::DBHandlerPtr& dbHandler) :
		_eventProvider(dbHandler), _dbHandler(dbHandler) {
}

bool EventMgr::init() {
	if (!_dbHandler->createTable(db::EventModel())) {
		Log::error("Failed to create event table");
		return false;
	}
	if (!_dbHandler->createTable(db::EventPointModel())) {
		Log::error("Failed to create event point table");
		return false;
	}

	if (!_eventProvider.init()) {
		Log::error("Failed to init event provider");
		return false;
	}

	return true;
}

void EventMgr::update(long dt) {
	for (auto i = _events.begin(); i != _events.end(); ++i)  {
		if (i->second->update(dt)) {
			continue;
		}
		i->second->stop();
		i = _events.erase(i);
	}
}

void EventMgr::shutdown() {
	for (auto& e : _events) {
		e.second->stop();
	}
	_events.clear();
}

bool EventMgr::startEvent(EventId id) {
	const db::EventModelPtr& model = _eventProvider.get(id);
	if (!model) {
		Log::warn("Failed to get the event data with the id %i", (int)id);
		return false;
	}
	const EventPtr& event = std::make_shared<Event>(id);
	if (!event->start()) {
		Log::warn("Failed to start the event with the id %i", (int)id);
		return false;
	}
	_events.insert(std::make_pair(id, event));
	return true;
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
