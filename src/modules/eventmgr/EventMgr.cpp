/**
 * @file
 */

#include "EventMgr.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "io/Filesystem.h"
#include "LUAFunctions.h"
#include "EventMgrModels.h"
#include "persistence/Timestamp.h"

namespace eventmgr {

EventMgr::EventMgr(const EventProviderPtr& eventProvider, const core::TimeProviderPtr& timeProvider) :
		_eventProvider(eventProvider), _timeProvider(timeProvider) {
}

bool EventMgr::init(const core::String& luaScript) {
	if (!_eventProvider->init()) {
		Log::error("Failed to init event provider");
		return false;
	}

	luaL_Reg create = { "create", luaCreateEventConfigurationData };
	luaL_Reg eof = { nullptr, nullptr };
	luaL_Reg funcs[] = { create, eof };

	lua::LUAType luaEvent = _lua.registerType("EventConfigurationData");
	luaEvent.addFunction("type", luaEventConfigurationDataGetType);
	luaEvent.addFunction("name", luaEventConfigurationDataGetName);
	luaEvent.addFunction("__gc", luaEventConfigurationDataGC);
	luaEvent.addFunction("__tostring", luaEventConfigurationDataToString);

	_lua.reg("event", funcs);

	if (!_lua.load(luaScript)) {
		Log::error("%s", _lua.error().c_str());
		return false;
	}

	// loads all the event configurations
	_lua.newGlobalData<EventMgr>("EventMgr", this);
	if (!_lua.execute("init")) {
		Log::error("%s", _lua.error().c_str());
		return false;
	}

	return true;
}

void EventMgr::update(long dt) {
	core_trace_scoped(EventMgrUpdate);
	const auto currentMillis = _timeProvider->tickNow();
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
			Log::info("Stop event of type " PRIEventId, (EventId)data->id());
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

EventPtr EventMgr::createEvent(const core::String& nameId, EventId id) const {
	auto dataIter = _eventData.find(nameId);
	if (dataIter == _eventData.end()) {
		Log::warn("Can't start event with event id " PRIEventId ". No configuration found for %s",
				id, nameId.c_str());
		return EventPtr();
	}
	const EventConfigurationDataPtr& data = dataIter->second;
	switch (data->type) {
	case Type::GENERIC:
		return std::make_shared<Event>(id, data);
	case Type::NONE:
		break;
	}
	return EventPtr();
}

bool EventMgr::startEvent(const db::EventModelPtr& model) {
	const core::String& nameId = model->nameid();
	const EventId id = model->id();
	const EventPtr& event = createEvent(nameId, id);
	if (!event || !event->start()) {
		Log::warn("Failed to start the event with the id " PRIEventId, id);
		return false;
	}
	Log::info("Start event %s (id: " PRIEventId ")", nameId.c_str(), (EventId)model->id());
	Log::debug("Event start time %lu, end time: %lu",
			(unsigned long)model->startdate().millis(),
			(unsigned long)model->enddate().millis());
	_events.insert(std::make_pair(id, event));
	return true;
}

EventConfigurationDataPtr EventMgr::createEventConfig(const char *nameId, Type type) {
	const EventConfigurationDataPtr& ptr = std::make_shared<EventConfigurationData>(nameId, type);
	if (!_eventData.insert(std::make_pair(nameId, ptr)).second) {
		Log::debug("Could not add new event configuration with id: '%s'", nameId);
		return EventConfigurationDataPtr();
	}
	return ptr;
}

}
