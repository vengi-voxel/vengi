#include "ITimedNode.h"

#include <stdlib.h>

namespace ai {

#define NOTSTARTED -1

ITimedNode::ITimedNode(const std::string& name, const std::string& parameters, const ConditionPtr& condition) :
		TreeNode(name, parameters, condition), _timerMillis(NOTSTARTED) {
	if (!parameters.empty())
		_millis = ::atol(parameters.c_str());
	else
		_millis = 1000L;
}

ITimedNode::~ITimedNode() {
}

TreeNodeStatus ITimedNode::execute(const AIPtr& entity, int64_t deltaMillis) {
	const TreeNodeStatus result = TreeNode::execute(entity, deltaMillis);
	if (result == CANNOTEXECUTE)
		return CANNOTEXECUTE;

	if (_timerMillis == NOTSTARTED) {
		_timerMillis = _millis;
		const TreeNodeStatus status = executeStart(entity, deltaMillis);
		if (status == FINISHED)
			_timerMillis = NOTSTARTED;
		return state(entity, status);
	}

	if (_timerMillis - deltaMillis > 0) {
		_timerMillis -= deltaMillis;
		const TreeNodeStatus status = executeRunning(entity, deltaMillis);
		if (status == FINISHED)
			_timerMillis = NOTSTARTED;
		return state(entity, status);
	}

	_timerMillis = NOTSTARTED;
	return state(entity, executeExpired(entity, deltaMillis));
}

}
