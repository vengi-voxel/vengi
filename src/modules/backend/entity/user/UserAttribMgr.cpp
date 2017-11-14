/**
 * @file
 */

#include "UserAttribMgr.h"
#include "attrib/AttributeType.h"
#include "AttribModel.h"
#include "core/Log.h"

namespace backend {

UserAttribMgr::UserAttribMgr(EntityId userId,
		attrib::Attributes& attribs,
		const persistence::DBHandlerPtr& dbHandler) :
				_userId(userId), _attribs(attribs), _dbHandler(dbHandler) {
}

bool UserAttribMgr::init() {
	if (!_dbHandler->select(db::AttribModel(), db::DBConditionAttribUserid(_userId), [this] (db::AttribModel&& model) {
		const int32_t id = model.attribtype();
		const attrib::Type type = (attrib::Type)id;
		const double value = model.value();
		_attribs.setCurrent(type, value);
	})) {
		Log::warn("Could not load attributes for user " PRIEntId, _userId);
	}
	return true;
}

void UserAttribMgr::shutdown() {
	for (int type = int(attrib::Type::MIN); type <= int(attrib::Type::MAX); ++type) {
		const double value = _attribs.current((attrib::Type)type);
		db::AttribModel model;
		model.setAttribtype(type);
		model.setUserid(_userId);
		model.setValue(value);
		_dbHandler->insert(model);
	}
}

}
