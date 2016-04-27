#include "conditions/HasEnemies.h"
#include "AI.h"

namespace ai {

HasEnemies::HasEnemies(const std::string& parameters) :
		ICondition("HasEnemies", parameters) {
	if (_parameters.empty())
		_enemyCount = -1;
	else
		_enemyCount = std::stoi(_parameters);
}

bool HasEnemies::evaluate(const AIPtr& entity) {
	const AggroMgr& mgr = entity->getAggroMgr();
	if (_enemyCount == -1) {
		// TODO: check why boolean operator isn't working here
		const bool hasEnemy = mgr.getHighestEntry() != nullptr;
		return hasEnemy;
	}
	const int size = static_cast<int>(mgr.getEntries().size());
	return size >= _enemyCount;
}

CONDITION_FACTORY_IMPL(HasEnemies)

}
