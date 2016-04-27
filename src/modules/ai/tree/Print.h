#pragma once

#include "AI.h"
#include "tree/ITask.h"
#include "common/String.h"
#include <iostream>

namespace ai {

/**
 * @brief The print task can print several states for an @code AI entity or just some
 * arbitrary string. Special commands are started with @code '::'.
 *
 * Available parameters:
 *   - ::tree
 *     Will print the current behaviour tree of the entity
 *   - ::attributes
 *     Will print the entity attributes
 *
 * Every other parameter will just be directly forwarded to be printed.
 */
class Print: public ITask {
private:
	void tree(const AIPtr& entity) const;
	void attributes(const AIPtr& entity) const;

	TreeNodeStatus handleCommand(const AIPtr& entity) const;

public:
	TASK_CLASS(Print)
	NODE_FACTORY

	TreeNodeStatus doAction(const AIPtr& entity, int64_t deltaMillis) override;
};

}
