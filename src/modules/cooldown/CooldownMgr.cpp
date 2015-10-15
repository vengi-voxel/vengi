#include "CooldownMgr.h"
#include "core/Common.h"

namespace cooldown {

namespace {
unsigned long durations[] = {
		0ul,
		15000ul,
		10000ul,
		100ul
};
CASSERT(lengthof(durations) == CooldownType::MAX);
}

CooldownMgr::CooldownMgr(const core::TimeProviderPtr& timeProvider) :
		_timeProvider(timeProvider), _lock("CooldownMgr") {
}

CooldownTriggerState CooldownMgr::triggerCooldown(CooldownType type) {
	core::ScopedWriteLock lock(_lock);
	CooldownPtr cooldown = _cooldowns[type];
	if (!cooldown) {
		cooldown = CooldownPtr(new Cooldown(type, defaultDuration(type), _timeProvider));
		_cooldowns[type] = cooldown;
	} else if (cooldown->running()) {
		Log::error("Failed to trigger the cooldown of type %i: already running", type);
		return CooldownTriggerState::ALREADY_RUNNING;
	}
	cooldown->start();
	_queue.push(cooldown);
	Log::debug("Triggered the cooldown of type %i (expires in %lims, started at %li)",
			type, cooldown->duration(), cooldown->startMillis());
	return CooldownTriggerState::SUCCESS;
}

CooldownPtr CooldownMgr::cooldown(CooldownType type) const {
	core::ScopedReadLock lock(_lock);
	auto i = _cooldowns.find(type);
	if (i == _cooldowns.end())
		return CooldownPtr();
	return i->second;
}

unsigned long CooldownMgr::defaultDuration(CooldownType type) const {
	return durations[type];
}

bool CooldownMgr::resetCooldown(CooldownType type) {
	const CooldownPtr& c = cooldown(type);
	if (!c) {
		return false;
	}
	c->reset();
	return true;
}

bool CooldownMgr::cancelCooldown(CooldownType type) {
	const CooldownPtr& c = cooldown(type);
	if (!c) {
		return false;
	}
	c->cancel();
	return true;
}

bool CooldownMgr::isCooldown(CooldownType type) {
	const CooldownPtr& c = cooldown(type);
	if (!c || !c->running()) {
		Log::trace("Cooldown of type %i is not running", type);
		return false;
	}
	Log::debug("Cooldown of type %i is running and has a runtime of %lims", type, c->duration());
	return true;
}

void CooldownMgr::update() {
	for (;;) {
		_lock.lockRead();
		if (_queue.empty()) {
			_lock.unlockRead();
			break;
		}
		CooldownPtr cooldown = _queue.top();
		_lock.unlockRead();
		if (cooldown->running()) {
			break;
		}

		_lock.lockWrite();
		_queue.pop();
		_lock.unlockWrite();
		Log::debug("Cooldown of type %i has just expired at %li", cooldown->type(), _timeProvider->tickTime());
		cooldown->expire();
	}
}
}
