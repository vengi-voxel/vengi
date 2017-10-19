/**
 * @file
 */

#include "SQLGenerator.h"
#include "core/String.h"
#include "core/Array.h"
#include "core/Log.h"
#include "core/Common.h"
#include "Model.h"
#include "DBCondition.h"
#include "OrderBy.h"

namespace persistence {

static const char *OperatorStrings[] = {
	" + ",
	" - ",
	" = "
};
static_assert(lengthof(OperatorStrings) == (int)persistence::Operator::MAX, "Invalid operator mapping");

static const char *OrderStrings[] = {
	"ASC",
	"DESC"
};
static_assert(lengthof(OrderStrings) == (int)persistence::Order::MAX, "Invalid order mapping");

static inline std::string quote(const std::string& in) {
	return core::string::format("\"%s\"", in.c_str());
}

static inline bool placeholder(const Model& model, const Field& field, std::stringstream& ss, int count) {
	if (field.type == FieldType::TIMESTAMP) {
		const Timestamp& ts = model.getValue<Timestamp>(field);
		if (ts.isNow()) {
			ss << "NOW()";
			return false;
		}
		ss << "to_timestamp($" << count << ")";
	} else {
		ss << "$" << count;
	}
	return true;
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
	case FieldType::BOOLEAN:
		return "BOOLEAN";
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
	case FieldType::SHORT:
		return "SMALLINT";
	case FieldType::BYTE:
		return "SMALLINT"; // TODO: check this
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

	for (const auto& f : table.fields()) {
		if (!f.isIndex()) {
			continue;
		}
		createTable << "CREATE INDEX IF NOT EXISTS " << table.tableName() << "_" << f.name << " ON ";
		createTable << table.tableName() << " USING btree (" << f.name << ");";
	}

	return createTable.str();
}

std::string createTruncateTableStatement(const Model& model) {
	return core::string::format("TRUNCATE TABLE \"%s\"", model.tableName().c_str());
}

std::string createDropTableStatement(const Model& model) {
	return core::string::format("DROP TABLE IF EXISTS \"%s\"", model.tableName().c_str());
}

std::string createUpdateStatement(const Model& model, BindParam* params) {
	const Fields& fields = model.fields();
	const std::string& tableName = model.tableName();
	std::stringstream update;
	update << "UPDATE " << quote(tableName) << " SET (";
	for (auto i = fields.begin(); i != fields.end(); ++i) {
		if (i->isPrimaryKey()) {
			continue;
		}
		const Field& f = *i;
		if (i != fields.begin()) {
			update << ", ";
		}
		update << quote(f.name);
	}
	update << ") = (";
	int index = 1;
	for (auto i = fields.begin(); i != fields.end(); ++i) {
		const Field& f = *i;
		if (f.isPrimaryKey()) {
			continue;
		}
		if (i != fields.begin()) {
			update << ", ";
		}
		if (placeholder(model, f, update, index)) {
			++index;
			if (params != nullptr) {
				params->push(model, f);
			}
		}
	}
	update << ") WHERE ";
	int where = 0;
	for (auto i = fields.begin(); i != fields.end(); ++i) {
		const Field& f = *i;
		if (!f.isPrimaryKey()) {
			continue;
		}
		if (where > 0) {
			update << " AND ";
		}
		++where;
		update << quote(f.name) << " = ";
		if (placeholder(model, f, update, index)) {
			++index;
			if (params != nullptr) {
				params->push(model, f);
			}
		}
	}

	return update.str();
}

std::string createDeleteStatement(const Model& table) {
	std::stringstream deleteStatement;
	deleteStatement << "DELETE FROM " << quote(table.tableName());
	return deleteStatement.str();
}

std::string createInsertStatement(const Model& model, BindParam* params) {
	std::stringstream insert;
	std::stringstream values;
	std::string autoincrement;
	std::string primaryKey;
	insert << "INSERT INTO " << quote(model.tableName()) << " (";
	int insertValueIndex = 1;
	int inserted = 0;
	for (const persistence::Field& f : model.fields()) {
		if (f.isPrimaryKey()) {
			primaryKey = f.name;
		}
		if (f.isAutoincrement()) {
			autoincrement = f.name;
			continue;
		}
		if (model.isNull(f)) {
			continue;
		}
		if (inserted > 0) {
			values << ", ";
			insert << ", ";
		}
		insert << quote(f.name);
		++inserted;
		if (placeholder(model, f, values, insertValueIndex)) {
			++insertValueIndex;
			if (params != nullptr) {
				params->push(model, f);
			}
		}
	}

	insert << ") VALUES (" << values.str() << ")";
	if (model.primaryKeys() == 1) {
		insert << " ON CONFLICT (" << quote(primaryKey);
		insert << ") DO UPDATE SET ";
		int fieldIndex = 0;
		for (const persistence::Field& f : model.fields()) {
			if (f.isPrimaryKey() || f.isAutoincrement()) {
				continue;
			}
			if (fieldIndex > 0) {
				insert << ", ";
			}
			insert << quote(f.name) << OperatorStrings[(int)f.updateOperator];
			if (placeholder(model, f, insert, insertValueIndex)) {
				++insertValueIndex;
				if (params != nullptr) {
					params->push(model, f);
				}
			}
			++fieldIndex;
		}
	}
	const UniqueKeys uniqueKeys = model.uniqueKeys();
	for (const auto& set : uniqueKeys) {
		insert << " ON CONFLICT (";
		insert << core::string::join(set.begin(), set.end(), ", ", [] (const std::string& fieldName) {
			return quote(fieldName);
		});
		insert << ") DO UPDATE SET ";
		int fieldIndex = 0;
		for (const persistence::Field& f : model.fields()) {
			if (f.isPrimaryKey() || f.isAutoincrement()) {
				continue;
			}
			if (set.find(f.name) != set.end()) {
				continue;
			}
			if (fieldIndex > 0) {
				insert << ", ";
			}
			insert << quote(f.name);
			if (f.updateOperator != Operator::SET) {
				insert << " = " << model.tableName() << "." << quote(f.name);
				insert << OperatorStrings[(int)f.updateOperator];
			}
			if (placeholder(model, f, insert, insertValueIndex)) {
				++insertValueIndex;
				if (params != nullptr) {
					params->push(model, f);
				}
			}
			++fieldIndex;
		}
	}
	if (!autoincrement.empty()) {
		insert << " RETURNING " << quote(autoincrement);
	}
	return insert.str();
}

// https://www.postgresql.org/docs/current/static/functions-formatting.html
// https://www.postgresql.org/docs/current/static/functions-datetime.html
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
			select << " AT TIME ZONE 'UTC') AS bigint) AS " << quote(f.name);
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

std::string createOrderBy(const OrderBy& orderBy) {
	std::stringstream ss;
	ss << " ORDER BY " << quote(orderBy.fieldname) << " " << OrderStrings[std::enum_value(orderBy.order)];
	return ss.str();
}

std::string createLimitOffset(const Range& range) {
	if (range.limit <= 0 && range.offset <= 0) {
		return "";
	}
	std::stringstream ss;
	if (range.limit > 0) {
		ss << " LIMIT " << range.limit;
	}
	if (range.offset > 0) {
		ss << " OFFSET " << range.offset;
	}
	return ss.str();
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
