/**
 * @file
 */

#include "SQLGenerator.h"
#include "core/String.h"
#include "Model.h"
#include "DBCondition.h"

namespace persistence {

static inline std::string quote(const std::string& in) {
	return core::string::format("\"%s\"", in.c_str());
}

static inline void sep(std::stringstream& ss, int count) {
	ss << "$" << count;
}

static std::string getDbFlags(int numberPrimaryKeys, const Constraints& constraints, const Field& field) {
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

static std::string getDbType(const Field& field) {
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

std::string createCreateTableStatement(const Model& table) {
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

std::string createTruncateTableStatement(const Model& model) {
	return core::string::format("TRUNCATE TABLE \"%s\"", model.tableName().c_str());
}

std::string createUpdateStatement(const Model& table) {
	std::stringstream update;
	update << "UPDATE " << quote(table.tableName()) << " --- TODO";
	return update.str();
}

std::string createDeleteStatement(const Model& table) {
	std::stringstream deleteStatement;
	deleteStatement << "DELETE FROM " << quote(table.tableName());
	return deleteStatement.str();
}

std::string createInsertStatement(const Model& table) {
	std::stringstream insert;
	std::stringstream values;
	std::string autoincrement;
	insert << "INSERT INTO " << quote(table.tableName()) << " (";
	int insertValueIndex = 0;
	for (const persistence::Field& f : table.fields()) {
		if (f.isAutoincrement()) {
			autoincrement = f.name;
			continue;
		}
		if (insertValueIndex > 0) {
			values << ", ";
			insert << ", ";
		}
		++insertValueIndex;
		insert << quote(f.name);
		sep(values, insertValueIndex);
	}

	insert << ") VALUES (" << values.str() << ")";
	if (!autoincrement.empty()) {
		insert << " RETURNING " << quote(autoincrement);
	}
	//ON CONFLICT (id) DO UPDATE
	//  SET column_1 = excluded.column_1,
	//      column_2 = excluded.column_2;
	return insert.str();
}

std::string createSelect(const Model& model) {
	const Fields& fields = model.fields();
	const std::string& tableName = model.tableName();
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

std::string createWhere(const DBCondition& condition, int &parameterCount) {
	std::stringstream where;
	where << " WHERE " << condition.statement(parameterCount);
	return where.str();
}

const char* createTransactionBegin() {
	return "START TRANSACTION";
}

const char* createTransactionCommit() {
	return "COMMIT";
}

const char* createTransactionRollback() {
	return "ROLLBACK";
}

}
