/**
 * @file
 */

#include "EventConfigurationData.h"
#include "Event.h"
#include "EventProvider.h"
#include "EventType.h"
#include "persistence/DBHandler.h"
#include "core/TimeProvider.h"
#include <memory>
#include <unordered_map>

namespace eventmgr {

class EventMgr {
private:
	std::unordered_map<Type, EventConfigurationData> _eventData;
	std::unordered_map<EventId, EventPtr> _events;

	EventProviderPtr _eventProvider;
	core::TimeProviderPtr _timeProvider;

	EventPtr createEvent(Type eventType, EventId id) const;

	bool startEvent(const db::EventModelPtr& model);
public:
	EventMgr(const EventProviderPtr& eventProvider, const core::TimeProviderPtr& timeProvider);

	bool init();
	void update(long dt);
	void shutdown();

	EventPtr runningEvent(EventId id) const;
	int runningEvents() const;
};

inline int EventMgr::runningEvents() const {
	return (int)_events.size();
}

typedef std::shared_ptr<EventMgr> EventMgrPtr;

}
