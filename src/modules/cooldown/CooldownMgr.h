/**
 * @file
 */

#pragma once

#include "core/ReadWriteLock.h"
#include "Cooldown.h"
#include "core/IComponent.h"
#include "core/TimeProvider.h"
#include "core/ReadWriteLock.h"
#include "CooldownProvider.h"

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
class CooldownMgr: public core::IComponent {
protected:
	core::TimeProviderPtr _timeProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	core::ReadWriteLock _lock;

	struct CooldownComparatorLess {
		inline bool operator()(const CooldownPtr& x, const CooldownPtr& y) const {
			return std::less<Cooldown>()(*x.get(), *y.get());
		}
	};

	typedef std::priority_queue<CooldownPtr, std::vector<CooldownPtr>, CooldownComparatorLess> CooldownQueue;
	/**
	 * @brief Running cooldowns - sorted by expire time. There can only be one cooldown of the same
	 * type at the same time.
	 */
	CooldownQueue _queue;

	typedef std::unordered_map<Type, CooldownPtr, network::EnumHash<Type> > Cooldowns;
	/**
	 * @brief This is a pool of @c Cooldown instances.
	 */
	Cooldowns _cooldowns;

	/**
	 * @brief Create @c Cooldown instances for the pool
	 * @param[in] type The @c Type to start
	 * @param[in] startMillis The millisecond timestamp when the Cooldown was triggered.
	 * If this is less than @c 0 the @c TimeProvider will be used to resolve the time
	 */
	CooldownPtr createCooldown(Type type, long startMillis = -1l) const;
public:
	CooldownMgr(const core::TimeProviderPtr& timeProvider, const cooldown::CooldownProviderPtr& cooldownProvider);
	virtual ~CooldownMgr() {}

	/**
	 * @brief Tries to trigger the specified cooldown for the given entity
	 * @param[in] type The cooldown type to trigger - needed to lookup the cooldown data like expire times
	 * @param[in] callback A @c CooldownCallback that is called with the @c CallbackType parameter whenever the
	 * cooldown switches to a new state.
	 */
	virtual CooldownTriggerState triggerCooldown(Type type, CooldownCallback callback = CooldownCallback());

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

	virtual bool init() override {
		return true;
	}

	virtual void shutdown() override {
	}

	/**
	 * @brief Update cooldown states
	 */
	void update();
};

typedef std::shared_ptr<CooldownMgr> CooldownMgrPtr;

}
