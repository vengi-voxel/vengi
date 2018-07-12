/**
 * @file
 */

#include "PersistenceMgr.h"
#include "DBHandler.h"
#include "MassQuery.h"
#include "core/Common.h"
#include "core/Trace.h"

namespace persistence {

PersistenceMgr::PersistenceMgr(const DBHandlerPtr& dbHandler) :
		_lock("persistencemgr"), _dbHandler(dbHandler) {
}

bool PersistenceMgr::registerSavable(uint32_t fourcc, ISavable *savable) {
	Log::trace(logid, "Register savable (fourcc: %u, savable: %p)", fourcc, savable);
	core::ScopedWriteLock lock(_lock);
	_savables[fourcc].insert(savable);
	return true;
}

bool PersistenceMgr::unregisterSavable(uint32_t fourcc, ISavable *savable) {
	core_trace_scoped(PersistenceMgrUnregisterSavable);
	Log::trace(logid, "Unregister savable (fourcc: %u, savable: %p)", fourcc, savable);
	core::ScopedWriteLock lock(_lock);
	auto i = _savables.find(fourcc);
	if (i == _savables.end()) {
		Log::trace(logid, "Could not find fourcc (fourcc: %u, savable: %p)", fourcc, savable);
		return false;
	}
	auto s = i->second.find(savable);
	if (s != i->second.end()) {
		i->second.erase(s);
		// make sure to persist the dirty state
		MassQuery stmt = _dbHandler->massQuery();
		stmt.add(savable);
		stmt.commit();
		Log::trace(logid, "Removed savable (fourcc: %u, savable: %p)", fourcc, savable);
		return true;
	}
	Log::trace(logid, "Could not find savable (fourcc: %u, savable: %p)", fourcc, savable);
	return false;
}

bool PersistenceMgr::init() {
	return true;
}

void PersistenceMgr::shutdown() {
	core_trace_scoped(PersistenceMgrShutdown);
	update(0l);
	core::ScopedWriteLock lock(_lock);
	_savables.clear();
}

void PersistenceMgr::update(long dt) {
	core_trace_scoped(PersistenceMgrUpdate);
	core::ScopedReadLock lock(_lock);
	for (auto& collection : _savables) {
		if (collection.second.empty()) {
			continue;
		}
		MassQuery stmt = _dbHandler->massQuery();
		for (ISavable *savable : collection.second) {
			stmt.add(savable);
		}
		stmt.commit();
	}
	Log::debug(logid, "Persisted dirty states of %i savables", (int)_savables.size());
}

}
