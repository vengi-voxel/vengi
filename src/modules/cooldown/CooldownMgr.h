/**
 * @file
 */

#pragma once

#include "core/ReadWriteLock.h"
#include "Cooldown.h"
#include "core/NonCopyable.h"
#include "core/TimeProvider.h"
#include "core/ReadWriteLock.h"
#include "CooldownDuration.h"

#include <memory>
#include <unordered_map>
#include <queue>
#include <functional>
#include <vector>

namespace cooldown {

/**
 * @brief Cooldown manager that handles cooldowns for one entity
 * @ingroup Cooldowns
 */
class CooldownMgr: public core::NonCopyable {
private:
	core::TimeProviderPtr _timeProvider;
	cooldown::CooldownDurationPtr _cooldownDuration;
	core::ReadWriteLock _lock;

	struct CooldownComparatorLess: public std::binary_function<CooldownPtr, CooldownPtr, bool> {
		inline bool operator()(const CooldownPtr x, const CooldownPtr y) const {
			return std::less<Cooldown>()(*x.get(), *y.get());
		}
	};

	typedef std::priority_queue<CooldownPtr, std::vector<CooldownPtr>, CooldownComparatorLess> CooldownQueue;
	CooldownQueue _queue;
	typedef std::unordered_map<Type, CooldownPtr, network::EnumHash<Type> > Cooldowns;
	Cooldowns _cooldowns;
public:
	CooldownMgr(const core::TimeProviderPtr& timeProvider, const cooldown::CooldownDurationPtr& cooldownDuration);

	/**
	 * @brief Tries to trigger the specified cooldown for the given entity
	 */
	CooldownTriggerState triggerCooldown(Type type);

	/**
	 * @brief Reset a cooldown and restart it
	 */
	bool resetCooldown(Type type);

	unsigned long defaultDuration(Type type) const;
	CooldownPtr cooldown(Type type) const;

	/**
	 * @brief Cancel an already running cooldown
	 */
	bool cancelCooldown(Type type);

	/**
	 * @brief Checks whether a user has the given cooldown running
	 */
	bool isCooldown(Type type);

	/**
	 * @brief Update cooldown states
	 */
	void update();
};

typedef std::shared_ptr<CooldownMgr> CooldownMgrPtr;

}
