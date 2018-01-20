/**
 * @file
 */

#include "UserAttribMgr.h"
#include "attrib/AttributeType.h"
#include "AttribModel.h"
#include "core/Log.h"

namespace backend {

static constexpr uint32_t FOURCC = FourCC('A','T','T','R');

UserAttribMgr::UserAttribMgr(EntityId userId,
		attrib::Attributes& attribs,
		const persistence::DBHandlerPtr& dbHandler,
		const persistence::PersistenceMgrPtr& persistenceMgr) :
				_userId(userId), _attribs(attribs), _dbHandler(dbHandler), _persistenceMgr(persistenceMgr) {
	_attribs.addListener(std::bind(&UserAttribMgr::onAttribChange, this, std::placeholders::_1));
}

void UserAttribMgr::onAttribChange(const attrib::DirtyValue& v) {
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
	_dirtyModels.resize(int(attrib::Type::MAX));
	_persistenceMgr->registerSavable(FOURCC, this);
	return true;
}

void UserAttribMgr::shutdown() {
	_persistenceMgr->unregisterSavable(FOURCC, this);
}

bool UserAttribMgr::getDirtyModels(Models& models) {
	Collection::underlying_type c;
	_dirtyAttributeTypes.swap(c);
	for (const attrib::DirtyValue& v : c) {
		// we only persist current values, max values are given by containers.
		if (!v.current) {
			continue;
		}
		db::AttribModel& model = _dirtyModels[int(v.type)];
		model.setAttribtype(v.type);
		model.setUserid(_userId);
		model.setValue(v.value);
		models.push_back(&model);
	};
	return true;
}

}
