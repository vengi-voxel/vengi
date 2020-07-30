/**
 * @file
 */
#pragma once

#include "TreeNode.h"

namespace backend {

#define TIMERNODE_CLASS(NodeName) \
	NodeName(const core::String& name, const core::String& parameters, const ConditionPtr& condition) : \
		ITimedNode(name, parameters, condition) { \
		_type = CORE_STRINGIFY(NodeName); \
	} \
	virtual ~NodeName() { \
	} \
	\
	NODE_FACTORY(NodeName)

/**
 * @brief A timed node is a @c TreeNode that is executed until a given time (millis) is elapsed.
 */
class ITimedNode : public TreeNode {
protected:
	int64_t _timerMillis;
	int64_t _millis;
public:
	ITimedNode(const core::String& name, const core::String& parameters, const ConditionPtr& condition);
	virtual ~ITimedNode() {}

	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override;

	/**
	 * @brief Called whenever the timer is started or restarted
	 * @note The returned @c TreeNodeStatus is recorded automatically
	 */
	virtual ai::TreeNodeStatus executeStart(const AIPtr& /*entity*/, int64_t /*deltaMillis*/) {
		return ai::TreeNodeStatus::RUNNING;
	}

	/**
	 * @brief Called whenever the timer is running. Not called in the frame where the timer
	 * is started or in the frame where it expired.
	 * @note If you have a timer started, don't get into the timer callbacks for some time (e.g.
	 * the attached @c ICondition evaluation prevents the action from being executed), you will
	 * not get into @c executeRunning, but directly into @c executeExpired.
	 * @note The returned @c TreeNodeStatus is recorded automatically
	 */
	virtual ai::TreeNodeStatus executeRunning(const AIPtr& /*entity*/, int64_t /*deltaMillis*/) {
		return ai::TreeNodeStatus::RUNNING;
	}

	/**
	 * @brief Called in the frame where the timer expired.
	 * @note The returned @c TreeNodeStatus is recorded automatically
	 */
	virtual ai::TreeNodeStatus executeExpired(const AIPtr& /*entity*/, int64_t /*deltaMillis*/) {
		return ai::TreeNodeStatus::FINISHED;
	}
};

}
