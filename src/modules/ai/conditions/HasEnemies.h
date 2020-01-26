/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "conditions/ICondition.h"
#include "common/StringUtil.h"
#include "aggro/AggroMgr.h"

namespace ai {

/**
 * @brief This condition checks whether there are enemies.
 *
 * You can either check whether there are enemies at all. Or whether there are more than x enemies.
 * The answer is given with the @c AggroMgr. So keep in mind that this might even return @c true in
 * those cases where the enemy is maybe no longer available. It all depends on how you use the
 * @c AggroMgr or how your aggro @c Entry reduce is configured.
 */
class HasEnemies: public ICondition {
protected:
	int _enemyCount;
public:
	CONDITION_FACTORY(HasEnemies)

	explicit HasEnemies(const core::String& parameters) :
			ICondition("HasEnemies", parameters) {
		if (_parameters.empty()) {
			_enemyCount = -1;
		} else {
			_enemyCount = core::string::toInt(_parameters);
		}
	}

	bool evaluate(const AIPtr& entity) override {
		const AggroMgr& mgr = entity->getAggroMgr();
		if (_enemyCount == -1) {
			// TODO: check why boolean operator isn't working here
			const bool hasEnemy = mgr.getHighestEntry() != nullptr;
			return hasEnemy;
		}
		const int size = (int)mgr.count();
		return size >= _enemyCount;
	}
};

}
