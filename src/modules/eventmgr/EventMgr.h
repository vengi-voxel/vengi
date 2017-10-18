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

/**
 * @defgroup Events
 * @{
 */

/**
 * @brief The event manager deals with starting, ticking and ending game events.
 *
 * @todo if a event was stopped, this must be persisted in the event table - skip those in the event provider.
 *       if the endtime has passed, but that flag is not set, just load the event as usual and stop it in the next frame
 *       this is needed if the server had a downtime while the event would have ended. In such a case, no loot would
 *       get hand out to the players. To work around this, we let the event just restore all its states and then stop
 *       properly.
 */
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
	/**
	 * @brief Call this in your main loop
	 * Starts all events that are configured to run at the current time of the @c core::TimeProvider
	 */
	void update(long dt);
	/**
	 * @brief Call this when you shut down the application. It will inform the running events to properly shut down.
	 * This is useful if you plan to restore the state of an event after your start the application again.
	 */
	void shutdown();

	/**
	 * @brief Access to a running event identified by its @c EventId
	 */
	EventPtr runningEvent(EventId id) const;
	/**
	 * @return The amount of currently active/running events
	 */
	int runningEvents() const;
};

inline int EventMgr::runningEvents() const {
	return (int)_events.size();
}

typedef std::shared_ptr<EventMgr> EventMgrPtr;

/**
 * @}
 */

}
