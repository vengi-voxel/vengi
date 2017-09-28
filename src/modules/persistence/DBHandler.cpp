/**
 * @file
 */

#include "DBHandler.h"
#include "core/Singleton.h"
#include "ConnectionPool.h"
#include "core/Log.h"
#include "core/String.h"
#include <sstream>

// TODO: remove me? all in state?
#include <libpq-fe.h>

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

std::string DBHandler::getDbType(const Field& field) {
	if (field.type == FieldType::PASSWORD
	 || field.type == FieldType::STRING) {
		if (field.length > 0) {
			return core::string::format("VARCHAR(%i)", field.length);
		}
		return "VARCHAR(256)";
	}
	if (field.length > 0) {
		Log::warn("Ignoring field length for '%s'", field.name.c_str());
	}

	switch (field.type) {
	case FieldType::TEXT:
		return "TEXT";
	case FieldType::TIMESTAMP:
		return "TIMESTAMP";
	case FieldType::LONG:
		if (field.isAutoincrement()) {
			return "";
		}
		return "BIGINT";
	case FieldType::INT:
		if (field.isAutoincrement()) {
			return "";
		}
		return "INT";
	case FieldType::STRING:
	case FieldType::PASSWORD:
	case FieldType::MAX:
		break;
	}
	return "";
}

std::string DBHandler::createCreateTableStatement(const Model& table) {
	std::stringstream createTable;
	createTable << "CREATE TABLE IF NOT EXISTS " << quote(table.tableName()) << " (";
	bool firstField = true;
	for (const auto& f : table.fields()) {
		if (!firstField) {
			createTable << ", ";
		}
		createTable << quote(f.name);
		const std::string& dbType = getDbType(f);
		if (!dbType.empty()) {
			createTable << " " << dbType;
		}
		const std::string& flags = getDbFlags(table.primaryKeys(), table.constraints(), f);
		if (!flags.empty()) {
			createTable << " " << flags;
		}
		firstField = false;
	}

	if (!table.uniqueKeys().empty()) {
		bool firstUniqueKey = true;
		for (const auto& uniqueKey : table.uniqueKeys()) {
			createTable << ", UNIQUE(";
			for (const std::string& fieldName : uniqueKey) {
				if (!firstUniqueKey) {
					createTable << ", ";
				}
				createTable << quote(fieldName);
				firstUniqueKey = false;
			}
			createTable << ")";
		}
	}

	if (table.primaryKeys() > 1) {
		createTable << ", PRIMARY KEY(";
		bool firstPrimaryKey = true;
		for (const auto& f : table.fields()) {
			if (!f.isPrimaryKey()) {
				continue;
			}
			if (!firstPrimaryKey) {
				createTable << ", ";
			}
			createTable << quote(f.name);
			firstPrimaryKey = false;
		}
		createTable << ")";
	}
	createTable << ");";
	return createTable.str();
}

std::string DBHandler::getDbFlags(int numberPrimaryKeys, const Constraints& constraints, const Field& field) {
	std::stringstream ss;
	bool empty = true;
	if (field.isAutoincrement()) {
		empty = false;
		if (field.type == FieldType::LONG) {
			ss << "BIGSERIAL";
		} else {
			ss << "SERIAL";
		}
	}
	if (field.isNotNull()) {
		if (!empty) {
			ss << " ";
		}
		ss << "NOT NULL";
		empty = false;
	}
	if (field.isPrimaryKey() && numberPrimaryKeys == 1) {
		if (!empty) {
			ss << " ";
		}
		ss << "PRIMARY KEY";
		empty = false;
	}
	if (field.isUnique()) {
		auto i = constraints.find(field.name);
		// only if there is one field in the unique list - otherwise we have to construct
		// them differently like the primary key for multiple fields
		if (i == constraints.end() || i->second.fields.size() == 1) {
			if (!empty) {
				ss << " ";
			}
			ss << "UNIQUE";
			empty = false;
		}
	}
	if (!field.defaultVal.empty()) {
		if (!empty) {
			ss << " ";
		}
		ss << "DEFAULT " << field.defaultVal;
		empty = false;
	}
	return ss.str();
}

std::string DBHandler::createSelect(const Model& model) {
	const Fields& fields = model.fields();
	return createSelect(fields, model.tableName());
}

std::string DBHandler::createSelect(const Fields& fields, const std::string& tableName) {
	std::stringstream select;
	select << "SELECT ";
	for (auto i = fields.begin(); i != fields.end(); ++i) {
		const Field& f = *i;
		if (i != fields.begin()) {
			select << ", ";
		}
		if (f.type == FieldType::TIMESTAMP) {
			select << "CAST(EXTRACT(EPOCH FROM ";
		}
		select << quote(f.name);
		if (f.type == FieldType::TIMESTAMP) {
			select << " AT TIME ZONE 'UTC') AS bigint) * 1000 AS " << quote(f.name);
		}
	}

	select << " FROM " << quote(tableName) << "";
	return select.str();
}

std::string DBHandler::quote(const std::string& in) {
	return core::string::format("\"%s\"", in.c_str());
}

void DBHandler::truncate(const Model& model) const {
	model.exec("TRUNCATE TABLE " + quote(model.tableName()));
}

void DBHandler::truncate(Model&& model) const {
	model.exec("TRUNCATE TABLE " + quote(model.tableName()));
}

bool DBHandler::createTable(Model&& model) const {
	const std::string& query = createCreateTableStatement(model);
	return exec(query);
}

bool DBHandler::exec(const std::string& query) const {
	const State& s = execInternal(query);
	return s.result;
}

State DBHandler::execInternal(const std::string& query) const {
	ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
	if (!scoped) {
		Log::error("Could not execute query '%s' - could not acquire connection", query.c_str());
		return State();
	}
	ConnectionType* conn = scoped.connection()->connection();
	State s(conn, PQexec(conn, query.c_str()));
	if (!s.result) {
		Log::warn("Failed to execute query: '%s'", query.c_str());
	} else {
		Log::debug("Executed query: '%s'", query.c_str());
	}
	return s;
}

}
