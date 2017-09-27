/**
 * @file
 */

#include "EventConfigurationData.h"
#include "Event.h"
#include "EventProvider.h"
#include "EventType.h"
#include "persistence/DBHandler.h"
#include <memory>
#include <unordered_map>

namespace eventmgr {

class EventMgr {
private:
	std::unordered_map<Type, EventConfigurationData> _eventData;
	std::unordered_map<EventId, EventPtr> _events;

	EventProvider _eventProvider;
	persistence::DBHandlerPtr _dbHandler;
public:
	EventMgr(const persistence::DBHandlerPtr& dbHandler);

	bool init();

	void update(long dt);

	void shutdown();

	bool startEvent(EventId id);

	bool stopEvent(EventId id);
};

typedef std::shared_ptr<EventMgr> EventMgrPtr;

}
