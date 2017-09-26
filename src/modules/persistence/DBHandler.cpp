/**
 * @file
 */

#include "DBHandler.h"
#include "core/Singleton.h"
#include "ConnectionPool.h"
#include "core/Log.h"
#include <sstream>

namespace persistence {

DBHandler::DBHandler() {
}

bool DBHandler::init() {
	if (core::Singleton<ConnectionPool>::getInstance().init() <= 0) {
		Log::error("Failed to init the connection pool");
		return false;
	}
	return true;
}

void DBHandler::shutdown() {
	core::Singleton<ConnectionPool>::getInstance().shutdown();
}

std::string DBHandler::createSelect(const Model& model) {
	const Model::Fields& fields = model.fields();
	return createSelect(fields, model.tableName());
}

std::string DBHandler::createSelect(const Model::Fields& fields, const std::string& tableName) {
	std::stringstream select;
	select << "SELECT ";
	for (auto i = fields.begin(); i != fields.end(); ++i) {
		const Model::Field& f = *i;
		if (i != fields.begin()) {
			select << ", ";
		}
		if (f.type == Model::FieldType::TIMESTAMP) {
			select << "CAST(EXTRACT(EPOCH FROM ";
		}
		select << quote(f.name);
		if (f.type == Model::FieldType::TIMESTAMP) {
			select << " AT TIME ZONE 'UTC') AS bigint) * 1000 AS " << quote(f.name);
		}
	}

	select << " FROM " << quote(tableName) << "";
	return select.str();
}

}
