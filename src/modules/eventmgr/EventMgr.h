/**
 * @file
 */

#include <memory>

namespace eventmgr {

class EventMgr {
public:
	EventMgr();

	bool init();

	void shutdown();
};

typedef std::shared_ptr<EventMgr> EventMgrPtr;

}
