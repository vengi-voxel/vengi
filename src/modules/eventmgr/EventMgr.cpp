/**
 * @file
 */

#include "EventMgr.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Trace.h"
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
	core_trace_scoped(EventMgrUpdate);
	const auto currentMillis = _timeProvider->tickMillis();
	const EventProvider::EventData& eventData = _eventProvider->eventData();
	for (const auto& entry : eventData) {
		const db::EventModelPtr& data = entry.second;
		const auto i = _events.find(data->id());
		const persistence::Timestamp& endTime = data->enddate();
		if (endTime.millis() < currentMillis) {
			continue;
		}
		if (i == _events.end()) {
			const persistence::Timestamp& startTime = data->startdate();
			const auto eventStartMillis = startTime.millis();
			const long delta = eventStartMillis - currentMillis;
			if (delta <= 0) {
				core_trace_scoped(EventStart);
				startEvent(data);
			}
			continue;
		}

		const auto eventEndMillis = endTime.millis();
		if (eventEndMillis <= currentMillis) {
			core_trace_scoped(EventStop);
			const EventPtr& event = i->second;
			Log::info("Stop event of type %i", (int)data->id());
			event->stop();
			_events.erase(i);
			continue;
		}
	}
	for (auto i = _events.begin(); i != _events.end(); ++i)  {
		Log::debug("Tick event %i", (int)i->first);
		core_trace_scoped(EventUpdate);
		i->second->update(dt);
	}
}

EventPtr EventMgr::runningEvent(EventId id) const {
	auto i = _events.find(id);
	if (i == _events.end()) {
		return EventPtr();
	}
	return i->second;
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
