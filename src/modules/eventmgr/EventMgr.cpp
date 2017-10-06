/**
 * @file
 */

#include "EventMgr.h"
#include "core/Log.h"
#include "core/Common.h"
#include "EventMgrModels.h"

namespace eventmgr {

EventMgr::EventMgr(const EventProviderPtr& eventProvider, const core::TimeProviderPtr& timeProvider) :
		_eventProvider(eventProvider), _timeProvider(timeProvider) {
}

bool EventMgr::init() {
	if (!_eventProvider->init()) {
		Log::error("Failed to init event provider");
		return false;
	}

	return true;
}

void EventMgr::update(long dt) {
	const auto currentMillis = _timeProvider->tickTime();
	const EventProvider::EventData& eventData = _eventProvider->eventData();
	for (const auto& entry : eventData) {
		const db::EventModelPtr& data = entry.second;
		const auto i = _events.find(data->id());
		if (i == _events.end()) {
			const persistence::Timestamp& startTime = data->startdate();
			if (startTime.millis() >= currentMillis) {
				startEvent(data);
			}
			continue;
		}

		const persistence::Timestamp& endTime = data->enddate();
		if (endTime.millis() <= currentMillis) {
			const EventPtr& event = i->second;
			Log::info("Stop event of type %i", (int)data->id());
			event->stop();
			_events.erase(i);
			continue;
		}
	}
	for (auto i = _events.begin(); i != _events.end(); ++i)  {
		Log::info("Tick event %i", (int)i->first);
		i->second->update(dt);
	}
}

void EventMgr::shutdown() {
	for (auto& e : _events) {
		e.second->shutdown();
	}
	_events.clear();
	_eventProvider->shutdown();
}

EventPtr EventMgr::createEvent(Type eventType, EventId id) const {
	switch (eventType) {
	case Type::GENERIC:
		return std::make_shared<Event>(id);
	case Type::NONE:
		break;
	}
	return EventPtr();
}

bool EventMgr::startEvent(const db::EventModelPtr& model) {
	const int64_t type = model->type();
	const EventId id = model->id();
	if (type < std::enum_value(Type::MIN) || type > std::enum_value(Type::MAX)) {
		Log::warn("Failed to get the event type from event data with the id %i (type: %i)",
				(int)id, (int)type);
		return false;
	}
	const Type eventType = network::EnumValuesEventType()[type];
	const EventPtr& event = createEvent(eventType, id);
	if (!event->start()) {
		Log::warn("Failed to start the event with the id %i", (int)id);
		return false;
	}
	Log::info("Start event of type %s (id: %i)", network::EnumNameEventType(eventType), (int)model->id());
	Log::debug("Event start time %lu, end time: %lu",
			(unsigned long)model->startdate().millis(),
			(unsigned long)model->enddate().millis());
	_events.insert(std::make_pair(id, std::move(event)));
	return true;
}

}
