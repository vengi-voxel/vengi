/**
 * @file
 */

#include "HasEnemies.h"
#include "backend/entity/ai/aggro/AggroMgr.h"
#include "backend/entity/ai/AI.h"
#include "core/StringUtil.h"

namespace backend {

HasEnemies::HasEnemies(const core::String& parameters) :
		ICondition("HasEnemies", parameters) {
	if (_parameters.empty()) {
		_enemyCount = -1;
	} else {
		_enemyCount = core::string::toInt(_parameters);
	}
}

bool HasEnemies::evaluate(const AIPtr& entity) {
	const AggroMgr& mgr = entity->getAggroMgr();
	if (_enemyCount == -1) {
		// TODO: check why boolean operator isn't working here
		const bool hasEnemy = mgr.getHighestEntry() != nullptr;
		return state(hasEnemy);
	}
	const int size = (int)mgr.count();
	return state(size >= _enemyCount);
}

}
