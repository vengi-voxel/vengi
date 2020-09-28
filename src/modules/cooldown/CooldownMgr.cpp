/**
 * @file
 */

#include "CooldownMgr.h"
#include "cooldown/CooldownTriggerState.h"
#include "core/Common.h"
#include "core/Singleton.h"
#include "core/Log.h"

namespace cooldown {

CooldownMgr::CooldownMgr(const core::TimeProviderPtr& timeProvider, const cooldown::CooldownProviderPtr& cooldownProvider) :
		_timeProvider(timeProvider), _cooldownProvider(cooldownProvider), _lock("CooldownMgr") {
}

CooldownPtr CooldownMgr::createCooldown(Type type, uint64_t startMillis) const {
	const uint64_t duration = defaultDuration(type);
	uint64_t expireMillis;
	if (startMillis <= 0l) {
		expireMillis = 0L;
		startMillis = 0L;
	} else {
		expireMillis = startMillis + duration;
	}
	return std::make_shared<Cooldown>(type, duration, _timeProvider, startMillis, expireMillis);
}

CooldownTriggerState CooldownMgr::triggerCooldown(Type type, const CooldownCallback& callback) {
	if (type == Type::NONE) {
		return CooldownTriggerState::FAILED;
	}
	core::ScopedWriteLock lock(_lock);
	CooldownPtr c = cooldown(type);
	if (!c) {
		c = createCooldown(type);
		_cooldowns.put(type, c);
	}
	if (c->running()) {
		Log::trace("Failed to trigger the cooldown of type %i: already running", core::enumVal(type));
		return CooldownTriggerState::ALREADY_RUNNING;
	}
	c->start(callback);
	_queue.push(c);
	Log::debug("Triggered the cooldown of type %i (expires in %lims, started at %li)",
			core::enumVal(type), c->duration(), c->startMillis());
	return CooldownTriggerState::SUCCESS;
}

CooldownPtr CooldownMgr::cooldown(Type type) const {
	core::ScopedReadLock lock(_lock);
	auto i = _cooldowns.find(type);
	if (i == _cooldowns.end()) {
		return CooldownPtr();
	}
	return i->second;
}

uint64_t CooldownMgr::defaultDuration(Type type) const {
	return _cooldownProvider->duration(type);
}

bool CooldownMgr::resetCooldown(Type type) {
	const CooldownPtr& c = cooldown(type);
	if (!c) {
		return false;
	}
	c->reset();
	return true;
}

bool CooldownMgr::cancelCooldown(Type type) {
	const CooldownPtr& c = cooldown(type);
	if (!c) {
		return false;
	}
	c->cancel();
	return true;
}

bool CooldownMgr::isCooldown(Type type) {
	const CooldownPtr& c = cooldown(type);
	if (!c || !c->running()) {
		Log::trace("Cooldown of type %i is not running", core::enumVal(type));
		return false;
	}
	Log::debug("Cooldown of type %i is running and has a runtime of %lims",
			core::enumVal(type), c->duration());
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
		Log::debug("Cooldown of type %i has just expired", core::enumVal(cooldown->type()));
		cooldown->expire();
	}
}
}
