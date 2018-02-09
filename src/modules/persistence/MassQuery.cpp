/**
 * @file
 */

#include "MassQuery.h"
#include "ISavable.h"
#include "DBHandler.h"
#include "SQLGenerator.h"
#include "core/Assert.h"

namespace persistence {

MassQuery::MassQuery(const DBHandler* dbHandler, size_t amount) :
		_dbHandler(dbHandler), _commitSize(amount) {
	_insertOrUpdate.reserve(_commitSize);
	_delete.reserve(_commitSize);
}

MassQuery::~MassQuery() {
	commit();
}

void MassQuery::commit() {
	// TODO: how to handle the error state here?
	if (!_insertOrUpdate.empty()) {
		_dbHandler->insert(_insertOrUpdate);
		_insertOrUpdate.clear();
	}
	if (!_delete.empty()) {
		_dbHandler->deleteModels(_delete);
		_delete.clear();
	}
}

void MassQuery::add(ISavable* savable) {
	core_assert(savable != nullptr);
	std::vector<const Model*> models;
	if (!savable->getDirtyModels(models)) {
		return;
	}
	for (const Model* m : models) {
		if (m->shouldBeDeleted()) {
			_delete.push_back(m);
		} else {
			_insertOrUpdate.push_back(m);
		}
	}
	if (_insertOrUpdate.size() + _delete.size() >= _commitSize) {
		commit();
	}
}

}
