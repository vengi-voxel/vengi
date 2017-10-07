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

// TODO: if a event was stopped, this must be persisted in the event table - skip those in the event provider.
//       if the endtime has passed, but that flag is not set, just load the event as usual and stop it in the next frame
//       this is needed if the server had a downtime while the event would have ended. In such a case, no loot would
//       get hand out to the players. To work around this, we let the event just restore all its states and then stop
//       properly.
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
