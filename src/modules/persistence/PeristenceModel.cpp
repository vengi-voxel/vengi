/**
 * @file
 */

#include "PeristenceModel.h"

namespace persistence {

PeristenceModel::PeristenceModel(const std::string& tableName) :
		_tableName(tableName) {
}

PeristenceModel::~PeristenceModel() {
}

}
