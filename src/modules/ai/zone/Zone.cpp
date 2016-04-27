#include "Zone.h"
#include "ICharacter.h"

namespace ai {

bool Zone::doAddAI(const AIPtr& ai) {
	if (ai == nullptr)
		return false;
	const CharacterId& id = ai->getCharacter()->getId();
	if (_ais.find(id) != _ais.end())
		return false;
	_ais.insert(std::make_pair(id, ai));
	ai->setZone(this);
	return true;
}

bool Zone::doRemoveAI(const AIPtr& ai) {
	if (!ai)
		return false;
	const CharacterId& id = ai->getCharacter()->getId();
	AIMapIter i = _ais.find(id);
	if (i == _ais.end())
		return false;
	i->second->setZone(nullptr);
	_groupManager.removeFromAllGroups(i->second);
	_ais.erase(i);
	return true;
}

bool Zone::doDestroyAI(const CharacterId& id) {
	AIMapIter i = _ais.find(id);
	if (i == _ais.end())
		return false;
	_ais.erase(i);
	return true;
}

bool Zone::addAI(const AIPtr& ai) {
	if (!ai)
		return false;
	ScopedWriteLock scopedLock(_scheduleLock);
	_scheduledAdd.push_back(ai);
	return true;
}

bool Zone::destroyAI(const CharacterId& id) {
	ScopedWriteLock scopedLock(_scheduleLock);
	_scheduledDestroy.push_back(id);
	return true;
}

bool Zone::removeAI(const AIPtr& ai) {
	if (!ai)
		return false;
	ScopedWriteLock scopedLock(_scheduleLock);
	_scheduledRemove.push_back(ai);
	return true;
}

void Zone::update(int64_t dt) {
	{
		ScopedWriteLock scopedLock(_scheduleLock);
		for (const AIPtr& ai : _scheduledAdd) {
			doAddAI(ai);
		}
		_scheduledAdd.clear();
		for (const AIPtr& ai : _scheduledRemove) {
			doRemoveAI(ai);
		}
		_scheduledRemove.clear();
		for (auto id : _scheduledDestroy) {
			doDestroyAI(id);
		}
		_scheduledDestroy.clear();
	}

	auto func = [&] (const AIPtr& ai) {
		if (ai->isPause())
			return;
		ai->update(dt, _debug);
		ai->getBehaviour()->execute(ai, dt);
	};
	executeParallel(func);
	_groupManager.update(dt);
}

}
