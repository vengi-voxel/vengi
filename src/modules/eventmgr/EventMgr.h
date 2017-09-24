/**
 * @file
 */

#include <memory>

namespace eventmgr {

class EventMgr {
public:
	EventMgr();

	bool init();

	void update(long dt);

	void shutdown();
};

typedef std::shared_ptr<EventMgr> EventMgrPtr;

}
