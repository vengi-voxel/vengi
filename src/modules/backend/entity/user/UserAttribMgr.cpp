/**
 * @file
 */

#include "UserAttribMgr.h"
#include "attrib/AttributeType.h"
#include "AttribModel.h"
#include "core/Log.h"
#include "persistence/PersistenceMgr.h"
#include "persistence/DBHandler.h"

namespace backend {

UserAttribMgr::UserAttribMgr(EntityId userId,
		attrib::Attributes& attribs,
		const persistence::DBHandlerPtr& dbHandler,
		const persistence::PersistenceMgrPtr& persistenceMgr) :
				_userId(userId), _attribs(attribs), _dbHandler(dbHandler), _persistenceMgr(persistenceMgr) {
	_attribs.addListener(std::bind(&UserAttribMgr::onAttribChange, this, std::placeholders::_1));
}

void UserAttribMgr::onAttribChange(const attrib::DirtyValue& v) {
	// only handle the current values here - the max values are handled by the
	// assigned containers and don't have to be persisted.
	if (!v.current) {
		return;
	}
	_dirtyAttributeTypes.insert(v);
}

bool UserAttribMgr::init() {
	if (!_dbHandler->select(db::AttribModel(), db::DBConditionAttribModelUserid(_userId), [this] (db::AttribModel&& model) {
		const int32_t id = model.attribtype();
		const attrib::Type type = (attrib::Type)id;
		const double value = model.value();
		_attribs.setCurrent(type, value);
	})) {
		Log::warn("Could not load attributes for user " PRIEntId, _userId);
	}

	// initialize the models
	const int maxDirtyModels = core::enumVal(attrib::Type::MAX);
	_dirtyModels.resize(maxDirtyModels + 1);
	for (int i = 0; i <= maxDirtyModels; ++i) {
		db::AttribModel& model = _dirtyModels[i];
		model.setAttribtype(i);
		model.setUserid(_userId);
	}
	_persistenceMgr->registerSavable(FOURCC, this);
	return true;
}

void UserAttribMgr::shutdown() {
	Log::info("Shutdown attribute manager for user " PRIEntId, _userId);
	_persistenceMgr->unregisterSavable(FOURCC, this);
}

bool UserAttribMgr::getDirtyModels(Models& models) {
	Collection::underlying_type c;
	_dirtyAttributeTypes.swap(c);
	if (c.empty()) {
		return false;
	}
	models.reserve(models.size() + c.size());
	for (const attrib::DirtyValue& v : c) {
		// we only persist current values, max values are given by containers.
		if (!v.current) {
			continue;
		}
		db::AttribModel& model = _dirtyModels[int(v.type)];
		model.setValue(v.value);
		models.push_back(&model);
	};
	return true;
}

}
