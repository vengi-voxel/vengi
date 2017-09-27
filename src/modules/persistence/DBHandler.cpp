/**
 * @file
 */

#include "DBHandler.h"
#include "core/Singleton.h"
#include "ConnectionPool.h"
#include "core/Log.h"
#include "core/String.h"
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

std::string DBHandler::getDbType(const Model::Field& field) {
	if (field.type == Model::FieldType::PASSWORD
	 || field.type == Model::FieldType::STRING) {
		if (field.length > 0) {
			return core::string::format("VARCHAR(%i)", field.length);
		}
		return "VARCHAR(256)";
	}
	if (field.length > 0) {
		Log::warn("Ignoring field length for '%s'", field.name.c_str());
	}

	switch (field.type) {
	case Model::FieldType::TEXT:
		return "TEXT";
	case Model::FieldType::TIMESTAMP:
		return "TIMESTAMP";
	case Model::FieldType::LONG:
		if (field.isAutoincrement()) {
			return "";
		}
		return "BIGINT";
	case Model::FieldType::INT:
		if (field.isAutoincrement()) {
			return "";
		}
		return "INT";
	case Model::FieldType::STRING:
	case Model::FieldType::PASSWORD:
	case Model::FieldType::MAX:
		break;
	}
	return "";
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
