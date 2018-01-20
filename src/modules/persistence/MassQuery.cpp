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
}

MassQuery::~MassQuery() {
	commit();
}

void MassQuery::commit() {
	if (_models.empty()) {
		return;
	}
	// TODO: how to handle the error state here?
	_dbHandler->insert(_models);
	_models.clear();
}

void MassQuery::add(ISavable* savable) {
	core_assert(savable != nullptr);
	std::vector<const Model*> models;
	if (!savable->getDirtyModels(models)) {
		return;
	}
	std::move(models.begin(), models.end(), std::back_inserter(_models));
	if (_models.size() >= _commitSize) {
		commit();
	}
}

}
