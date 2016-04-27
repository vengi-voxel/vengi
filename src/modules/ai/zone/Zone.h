#pragma once

#include "AI.h"
#include "group/GroupMgr.h"
#include "common/Thread.h"
#include "common/ThreadPool.h"
#include "common/Types.h"
#include "common/ExecutionTime.h"
#include <unordered_map>
#include <list>

namespace ai {

/**
 * @brief A zone represents one logical zone that groups AI instances.
 *
 * You have to update the AI instances in this zone in your tick by calling
 * @c Zone::update.
 *
 * Zones should have unique names - otherwise the @c Server won't be able to
 * select a particular @c Zone to debug it.
 */
class Zone {
public:
	typedef std::unordered_map<CharacterId, AIPtr> AIMap;
	typedef std::list<AIPtr> AIScheduleList;
	typedef std::list<AIPtr> AIScheduleConstList;
	typedef std::list<CharacterId> CharacterIdList;
	typedef AIMap::const_iterator AIMapConstIter;
	typedef AIMap::iterator AIMapIter;

protected:
	const std::string _name;
	AIMap _ais;
	AIScheduleList _scheduledAdd;
	AIScheduleConstList _scheduledRemove;
	CharacterIdList _scheduledDestroy;
	bool _debug;
	ReadWriteLock _lock = {"zone"};
	ReadWriteLock _scheduleLock = {"zone-schedulelock"};
	ai::GroupMgr _groupManager;
	mutable ThreadPool _threadPool;

	/**
	 * @brief called in the zone update to add new @c AI instances.
	 *
	 * @note Make sure to also call @c removeAI whenever you despawn the given @c AI instance
	 * @note This doesn't lock the zone - but because @c Zone::update already does it
	 */
	bool doAddAI(const AIPtr& ai);
	/**
	 * @note This doesn't lock the zone - but because @c Zone::update already does it
	 */
	bool doRemoveAI(const AIPtr& ai);
	/**
	 * @brief @c removeAI will access the character and the @c AI object, this method does not need access to the data anymore.
	 *
	 * @note That means, that this can be called in case the attached @c ICharacter instances or the @c AI instance itself is
	 * already invalid.
	 * @note This doesn't lock the zone - but because @c Zone::update already does it
	 */
	bool doDestroyAI(const CharacterId& id);

public:
	Zone(const std::string& name, int threadCount = std::min(1u, std::thread::hardware_concurrency())) :
			_name(name), _debug(false), _threadPool(threadCount) {
	}

	virtual ~Zone() {}

	/**
	 * @brief Update all the @c ICharacter and @c AI instances in this zone.
	 * @param dt Delta time in millis since the last update call happened
	 * @note You have to call this on your own.
	 */
	void update(int64_t dt);

	/**
	 * @brief If you need to add new @code AI entities to a zone from within the @code AI tick (e.g. spawning via behaviour
	 * tree) - then you need to schedule the spawn. Otherwise you will end up in a deadlock
	 *
	 * @note This does not lock the zone for writing but a dedicated schedule lock
	 */
	bool addAI(const AIPtr& ai);

	/**
	 * @brief Will trigger a removal of the specified @c AI instance in the next @c Zone::update call
	 *
	 * @note This does not lock the zone for writing but a dedicated schedule lock
	 */
	bool removeAI(const AIPtr& ai);

	/**
	 * @brief Will trigger a destroy of the specified @c AI instance in the next @c Zone::update call
	 * @sa destroyAI
	 * @note @c removeAI will access the character and the @c AI object, this method does not need access to the data anymore.
	 * That means, that this can be called in case the attached @c ICharacter instances or the @c AI instance itself is
	 * already invalid.
	 */
	bool destroyAI(const CharacterId& id);

	/**
	 * @brief Every zone has its own name that identifies it
	 */
	const std::string& getName() const;

	/**
	 * @brief Activate the debugging for this particular server instance
	 * @param[in] debug @c true if you want to activate the debugging and send
	 * the npc states of this server to all connected clients, or @c false if
	 * none of the managed entities is broadcasted.
	 */
	void setDebug(bool debug);
	bool isDebug () const;

	GroupMgr& getGroupMgr();

	const GroupMgr& getGroupMgr() const;

	/**
	 * @brief Lookup for a particular @c AI in the zone.
	 *
	 * @return empty @c AIPtr() in the case the given @c CharacterId wasn't found in this zone.
	 *
	 * @note This locks the zone for reading to perform the CharacterId lookup
	 */
	inline AIPtr getAI(CharacterId id) const {
		ScopedReadLock scopedLock(_lock);
		auto i = _ais.find(id);
		if (i == _ais.end())
			return AIPtr();
		const AIPtr& ai = i->second;
		return ai;
	}

	/**
	 * @brief Executes a lambda or functor for the given character
	 *
	 * @return @c true if the func is going to get called for the character, @c false if not
	 * e.g. in the case the given @c CharacterId wasn't found in this zone.
	 * @note This is executed in a thread pool - so make sure to synchronize your lambda or functor.
	 * We also don't wait for the functor or lambda here, we are scheduling it in a worker in the
	 * thread pool.
	 *
	 * @note This locks the zone for reading to perform the CharacterId lookup
	 */
	template<typename Func>
	inline bool executeAsync(CharacterId id, const Func& func) const {
		const AIPtr& ai = getAI(id);
		if (!ai)
			return false;
		executeAsync(ai, func);
		return true;
	}

	/**
	 * @brief Executes a lambda or functor for the given character
	 *
	 * @returns @c std::future with the result of @c func.
	 * @note This is executed in a thread pool - so make sure to synchronize your lambda or functor.
	 * We also don't wait for the functor or lambda here, we are scheduling it in a worker in the
	 * thread pool. If you want to wait - you have to use the returned future.
	 */
	template<typename Func>
	inline auto executeAsync(const AIPtr& ai, const Func& func) const
		-> std::future<typename std::result_of<Func(const AIPtr&)>::type> {
		return _threadPool.enqueue(func, ai);
	}

	template<typename Func>
	inline auto execute(const AIPtr& ai, const Func& func) const
		-> typename std::result_of<Func(const AIPtr&)>::type {
		return func(ai);
	}

	template<typename Func>
	inline auto execute(const AIPtr& ai, Func& func)
		-> typename std::result_of<Func(const AIPtr&)>::type {
		return func(ai);
	}

	/**
	 * @brief The given functor or lambda must be able to deal with invalid @c AIPtr instances
	 * @note It's possible that the given @c CharacterId can't be found in the @c Zone.
	 * @return the return value of your functor or lambda
	 */
	template<typename Func>
	inline auto execute(CharacterId id, const Func& func) const
		-> typename std::result_of<Func(const AIPtr&)>::type {
		return execute(getAI(id), func);
	}

	/**
	 * @brief The given functor or lambda must be able to deal with invalid @c AIPtr instances
	 * @note It's possible that the given @c CharacterId can't be found in the @c Zone.
	 * @return the return value of your functor or lambda
	 */
	template<typename Func>
	inline auto execute(CharacterId id, Func& func)
		-> typename std::result_of<Func(const AIPtr&)>::type {
		return execute(getAI(id), func);
	}

	/**
	 * @brief Executes a lambda or functor for all the @c AI instances in this zone
	 * @note This is executed in a thread pool - so make sure to synchronize your lambda or functor.
	 * We are waiting for the execution of this.
	 *
	 * @note This locks the zone for reading
	 */
	template<typename Func>
	void executeParallel(Func& func) {
		std::vector<std::future<void> > results;
		{
			ScopedReadLock scopedLock(_lock);
			for (auto i = _ais.begin(); i != _ais.end(); ++i) {
				const AIPtr& ai = i->second;
				results.emplace_back(executeAsync(ai, func));
			}
		}
		for (auto && result: results)
			result.wait();
	}

	/**
	 * @brief Executes a lambda or functor for all the @c AI instances in this zone.
	 * @note This is executed in a thread pool - so make sure to synchronize your lambda or functor.
	 * We are waiting for the execution of this.
	 *
	 * @note This locks the zone for reading
	 */
	template<typename Func>
	void executeParallel(const Func& func) const {
		std::vector<std::future<void> > results;
		{
			ScopedReadLock scopedLock(_lock);
			for (auto i = _ais.begin(); i != _ais.end(); ++i) {
				const AIPtr& ai = i->second;
				results.emplace_back(executeAsync(ai, func));
			}
		}
		for (auto && result: results)
			result.wait();
	}

	/**
	 * @brief Executes a lambda or functor for all the @c AI instances in this zone
	 * We are waiting for the execution of this.
	 *
	 * @note This locks the zone for reading
	 */
	template<typename Func>
	void execute(const Func& func) const {
		ScopedReadLock scopedLock(_lock);
		for (auto i = _ais.begin(); i != _ais.end(); ++i) {
			const AIPtr& ai = i->second;
			func(ai);
		}
	}

	/**
	 * @brief Executes a lambda or functor for all the @c AI instances in this zone
	 * We are waiting for the execution of this.
	 *
	 * @note This locks the zone for reading
	 */
	template<typename Func>
	void execute(Func& func) {
		ScopedReadLock scopedLock(_lock);
		for (auto i = _ais.begin(); i != _ais.end(); ++i) {
			const AIPtr& ai = i->second;
			func(ai);
		}
	}

	inline std::size_t size() const {
		ScopedReadLock scopedLock(_lock);
		return _ais.size();
	}
};

inline void Zone::setDebug (bool debug) {
	_debug = debug;
}

inline bool Zone::isDebug () const {
	return _debug;
}

inline const std::string& Zone::getName() const {
	return _name;
}

inline GroupMgr& Zone::getGroupMgr() {
	return _groupManager;
}

inline const GroupMgr& Zone::getGroupMgr() const {
	return _groupManager;
}

}
