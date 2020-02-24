/**
 * @file
 */

#pragma once

#include <memory>
#include <map>
#include <unordered_set>
#include "ISavable.h"
#include "DBHandler.h"
#include "core/IComponent.h"
#include "core/concurrent/ReadWriteLock.h"

namespace persistence {

/**
 * @brief This class is responsible for calling the update mechanisms for the single components of each player.
 * It will collect all database actions in prepared statements to write delta values into the database.
 * @note Your @c ISavable instances must be registered and unregistered.
 */
class PersistenceMgr : public core::IComponent {
private:
	static constexpr uint32_t logid = FourCC('P','E','R','M');
	using Savables = std::unordered_set<ISavable*>;
	using Map = std::map<uint32_t, Savables>;
	Map _savables;
	core::ReadWriteLock _lock;
	const DBHandlerPtr _dbHandler;
public:
	PersistenceMgr(const DBHandlerPtr& dbHandler);
	virtual ~PersistenceMgr() {}

	virtual bool registerSavable(uint32_t fourcc, ISavable *savable);
	virtual bool unregisterSavable(uint32_t fourcc, ISavable *savable);

	bool init() override;
	/**
	 * @note You have to make sure, that the update is not called anymore and also not called currently.
	 */
	void shutdown() override;

	void update(long dt);
};

typedef std::shared_ptr<PersistenceMgr> PersistenceMgrPtr;

}
