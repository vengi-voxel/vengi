/**
 * @file
 */

#include "Zone.h"
#include "core/Trace.h"

namespace ai {

AIPtr Zone::getAI(CharacterId id) const {
	ScopedReadLock scopedLock(_lock);
	auto i = _ais.find(id);
	if (i == _ais.end()) {
		return AIPtr();
	}
	const AIPtr& ai = i->second;
	return ai;
}

std::size_t Zone::size() const {
	ScopedReadLock scopedLock(_lock);
	return _ais.size();
}

bool Zone::doAddAI(const AIPtr& ai) {
	if (ai == nullptr) {
		return false;
	}
	const CharacterId& id = ai->getCharacter()->getId();
	if (_ais.find(id) != _ais.end()) {
		return false;
	}
	_ais.insert(std::make_pair(id, ai));
	ai->setZone(this);
	return true;
}

bool Zone::doRemoveAI(const AIPtr& ai) {
	if (!ai) {
		return false;
	}
	const CharacterId& id = ai->getCharacter()->getId();
	AIMapIter i = _ais.find(id);
	if (i == _ais.end()) {
		return false;
	}
	i->second->setZone(nullptr);
	_groupManager.removeFromAllGroups(i->second);
	_ais.erase(i);
	return true;
}

bool Zone::doDestroyAI(const CharacterId& id) {
	AIMapIter i = _ais.find(id);
	if (i == _ais.end()) {
		return false;
	}
	_ais.erase(i);
	return true;
}

bool Zone::addAI(const AIPtr& ai) {
	if (!ai) {
		return false;
	}
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
	if (!ai) {
		return false;
	}
	ScopedWriteLock scopedLock(_scheduleLock);
	_scheduledRemove.push_back(ai);
	return true;
}

void Zone::update(int64_t dt) {
	core_trace_scoped(ZoneUpdate);
	{
		AIScheduleList scheduledRemove;
		AIScheduleList scheduledAdd;
		CharacterIdList scheduledDestroy;
		{
			ScopedWriteLock scopedLock(_scheduleLock);
			scheduledAdd.swap(_scheduledAdd);
			scheduledRemove.swap(_scheduledRemove);
			scheduledDestroy.swap(_scheduledDestroy);
		}
		ScopedWriteLock scopedLock(_lock);
		for (const AIPtr& ai : scheduledAdd) {
			doAddAI(ai);
		}
		scheduledAdd.clear();
		for (const AIPtr& ai : scheduledRemove) {
			doRemoveAI(ai);
		}
		scheduledRemove.clear();
		for (auto id : scheduledDestroy) {
			doDestroyAI(id);
		}
		scheduledDestroy.clear();
	}

	auto func = [&] (const AIPtr& ai) {
		if (ai->isPause()) {
			return;
		}
		ai->update(dt, _debug);
		ai->getBehaviour()->execute(ai, dt);
	};
	executeParallel(func);
	_groupManager.update(dt);
}

}
