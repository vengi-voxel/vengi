/**
 * @file
 */

#pragma once

#include <memory>
#include <map>
#include <unordered_set>
#include "ISavable.h"
#include "DBHandler.h"
#include "core/ReadWriteLock.h"

namespace persistence {

/**
 * @brief This class is responsible for calling the update mechanisms for the single components of each player.
 * It will collect all database actions in prepared statements to write delta values into the database.
 * @note Your @c ISavable instances must be registered and unregistered.
 */
class PersistenceMgr {
private:
	using Savables = std::unordered_set<ISavable*>;
	using Map = std::map<uint32_t, Savables>;
	Map _savables;
	core::ReadWriteLock _lock;
	const DBHandlerPtr _dbHandler;
public:
	PersistenceMgr(const DBHandlerPtr& dbHandler);

	bool registerSavable(uint32_t fourcc, ISavable *savable);
	bool unregisterSavable(uint32_t fourcc, ISavable *savable);

	bool init();
	/**
	 * @note You have to make sure, that the update is not called anymore and also not called currently.
	 */
	void shutdown();

	void update(long dt);
};

typedef std::shared_ptr<PersistenceMgr> PersistenceMgrPtr;

}
