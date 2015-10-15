#pragma once

#include "core/ReadWriteLock.h"
#include "Cooldown.h"
#include "core/NonCopyable.h"
#include "core/TimeProvider.h"
#include "core/ReadWriteLock.h"

#include <memory>
#include <unordered_map>
#include <queue>
#include <functional>
#include <vector>

namespace cooldown {

/**
 * @brief Cooldown manager that handles cooldowns for one entity
 */
class CooldownMgr: public core::NonCopyable {
private:
	core::TimeProviderPtr _timeProvider;
	core::ReadWriteLock _lock;

	struct CooldownComparatorLess: public std::binary_function<CooldownPtr, CooldownPtr, bool> {
		inline bool operator()(const CooldownPtr x, const CooldownPtr y) const {
			return std::less<Cooldown>()(*x.get(), *y.get());
		}
	};

	typedef std::priority_queue<CooldownPtr, std::vector<CooldownPtr>, CooldownComparatorLess> CooldownQueue;
	CooldownQueue _queue;
	typedef std::unordered_map<CooldownType, CooldownPtr> Cooldowns;
	Cooldowns _cooldowns;
public:
	CooldownMgr(const core::TimeProviderPtr& timeProvider);

	/**
	 * @brief Tries to trigger the specified cooldown for the given entity
	 */
	CooldownTriggerState triggerCooldown(CooldownType type);

	/**
	 * @brief Reset a cooldown and restart it
	 */
	bool resetCooldown(CooldownType type);

	unsigned long defaultDuration(CooldownType type) const;
	CooldownPtr cooldown(CooldownType type) const;

	/**
	 * @brief Cancel an already running cooldown
	 */
	bool cancelCooldown(CooldownType type);

	/**
	 * @brief Checks whether a user has the given cooldown running
	 */
	bool isCooldown(CooldownType type);

	/**
	 * @brief Update cooldown states
	 */
	void update();
};

typedef std::shared_ptr<CooldownMgr> CooldownMgrPtr;

}
