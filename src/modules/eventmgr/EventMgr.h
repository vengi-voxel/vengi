/**
 * @file
 */

#include "EventConfigurationData.h"
#include "Event.h"
#include "EventProvider.h"
#include "EventType.h"
#include "persistence/DBHandler.h"
#include "commonlua/LUA.h"
#include "core/TimeProvider.h"
#include <memory>
#include <unordered_map>

namespace io {
class Filesystem;
typedef std::shared_ptr<Filesystem> FilesystemPtr;
}

namespace eventmgr {

/**
 * @defgroup Events
 * @{
 */

/**
 * @brief The event manager deals with starting, ticking and ending game events.
 */
class EventMgr {
private:
	std::unordered_map<core::String, EventConfigurationDataPtr> _eventData;
	std::unordered_map<EventId, EventPtr> _events;

	EventProviderPtr _eventProvider;
	core::TimeProviderPtr _timeProvider;
	lua::LUA _lua;

	EventPtr createEvent(const core::String& nameId, EventId id) const;

	bool startEvent(const db::EventModelPtr& model);
public:
	EventMgr(const EventProviderPtr& eventProvider, const core::TimeProviderPtr& timeProvider);

	bool init(const core::String& luaScript);
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

	EventConfigurationDataPtr createEventConfig(const char *nameId, Type type);
};

inline int EventMgr::runningEvents() const {
	return (int)_events.size();
}

typedef std::shared_ptr<EventMgr> EventMgrPtr;

/**
 * @}
 */

}
