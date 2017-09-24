/**
 * @file
 */

#include "Event.h"
#include "EventData.h"
#include "Shared_generated.h"
#include <memory>
#include <unordered_map>

namespace eventmgr {

class EventMgr {
private:
	std::unordered_map<network::EventType, EventData> _eventData;
	std::unordered_map<EventId, EventPtr> _events;
public:
	EventMgr();

	bool init();

	void update(long dt);

	void shutdown();

	bool startEvent(EventId id);

	bool stopEvent(EventId id);
};

typedef std::shared_ptr<EventMgr> EventMgrPtr;

}
