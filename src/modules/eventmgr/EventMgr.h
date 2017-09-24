/**
 * @file
 */

#include "EventData.h"
#include "Shared_generated.h"
#include <memory>
#include <unordered_map>

namespace eventmgr {

class EventMgr {
private:
	std::unordered_map<network::EventType, EventData> _events;
public:
	EventMgr();

	bool init();

	void update(long dt);

	void shutdown();
};

typedef std::shared_ptr<EventMgr> EventMgrPtr;

}
