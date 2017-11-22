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
#include <unordered_map>

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

static std::string getDbFlags(const std::string& tablename, int numberPrimaryKeys, const Constraints& constraints, const Field& field) {
	char buf[1024] = { '\0' };
	bool empty = true;
	if (field.isNotNull()) {
		if (!empty) {
			core::string::append(buf, sizeof(buf), " ");
		}
		core::string::append(buf, sizeof(buf), "NOT NULL");
		empty = false;
	}
	if (field.isPrimaryKey() && numberPrimaryKeys == 1) {
		if (!empty) {
			core::string::append(buf, sizeof(buf), " ");
		}
		core::string::append(buf, sizeof(buf), "PRIMARY KEY");
		empty = false;
	}
	if (field.isUnique()) {
		auto i = constraints.find(field.name);
		// only if there is one field in the unique list - otherwise we have to construct
		// them differently like the primary key for multiple fields
		if (i == constraints.end() || i->second.fields.size() == 1) {
			if (!empty) {
				core::string::append(buf, sizeof(buf), " ");
			}
			core::string::append(buf, sizeof(buf), "UNIQUE");
			empty = false;
		}
	}
	if (!field.defaultVal.empty()) {
		if (!empty) {
			core::string::append(buf, sizeof(buf), " ");
		}
		core::string::append(buf, sizeof(buf), "DEFAULT ");
		core::string::append(buf, sizeof(buf), field.defaultVal.c_str());
		empty = false;
	} else if ((field.contraintMask & (int)ConstraintType::AUTOINCREMENT) != 0) {
		if (!empty) {
			core::string::append(buf, sizeof(buf), " ");
		}
		core::string::append(buf, sizeof(buf), "DEFAULT nextval('");
		core::string::append(buf, sizeof(buf), tablename.c_str());
		core::string::append(buf, sizeof(buf), "_");
		core::string::append(buf, sizeof(buf), field.name.c_str());
		core::string::append(buf, sizeof(buf), "_seq'::regclass)");
	}
	return std::string(buf);
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
		return "BIGINT";
	case FieldType::DOUBLE:
		return "DOUBLE PRECISION";
	case FieldType::INT:
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

std::string createCreateTableStatement(const Model& table, bool useForeignKeys) {
	std::stringstream createTable;
	for (const auto& f : table.fields()) {
		if ((f.contraintMask & (int)ConstraintType::AUTOINCREMENT) == 0) {
			continue;
		}
		createTable << "CREATE SEQUENCE IF NOT EXISTS " << table.schema() << "." << table.tableName() << "_" << f.name;
		createTable << "_seq START " << table.autoIncrementStart() << ";";
	}

	createTable << "CREATE TABLE IF NOT EXISTS \"" << table.schema() << "\".\"" << table.fullTableName() << "\" (";
	bool firstField = true;
	for (const auto& f : table.fields()) {
		if (!firstField) {
			createTable << ", ";
		}
		createTable << "\"" << f.name << "\"";
		const std::string& dbType = getDbType(f);
		if (!dbType.empty()) {
			createTable << " " << dbType;
		}
		const std::string& flags = getDbFlags(table.tableName(), table.primaryKeys(), table.constraints(), f);
		if (!flags.empty()) {
			createTable << " " << flags;
		}
		firstField = false;
	}

	if (!table.uniqueKeys().empty()) {
		bool firstUniqueKey = true;
		for (const auto& uniqueKey : table.uniqueKeys()) {
			createTable << ", CONSTRAINT ";
			createTable << table.tableName() << "_" << core::string::join(uniqueKey.begin(), uniqueKey.end(), "_");
			createTable << " UNIQUE(";
			for (const std::string& fieldName : uniqueKey) {
				if (!firstUniqueKey) {
					createTable << ", ";
				}
				createTable << "\"" << fieldName << "\"";
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
			createTable << "\"" << f.name << "\"";
			firstPrimaryKey = false;
		}
		createTable << ")";
	}

	if (useForeignKeys) {
		for (const auto& foreignKey : table.foreignKeys()) {
			createTable << ", CONSTRAINT ";
			createTable << table.tableName() << "_" << foreignKey.second.table << "_" << foreignKey.second.field;
			createTable << " FOREIGN KEY(\"" << foreignKey.first << "\") REFERENCES \"";
			createTable << foreignKey.second.table << "\"(\"" << foreignKey.second.field;
			createTable << "\") MATCH SIMPLE ON UPDATE NO ACTION ON DELETE NO ACTION";
		}
	}

	createTable << ");";

	for (const auto& f : table.fields()) {
		if (!f.isIndex()) {
			continue;
		}
		createTable << "CREATE INDEX IF NOT EXISTS " << table.schema() << "." << table.tableName() << "_" << f.name << " ON \"";
		createTable << table.tableName() << "\" USING btree (\"" << f.name << "\");";
	}

	return createTable.str();
}

std::string createTruncateTableStatement(const Model& model) {
	return core::string::format("TRUNCATE TABLE \"%s\".\"%s\"", model.schema().c_str(), model.tableName().c_str());
}

std::string createDropTableStatement(const Model& model) {
	std::stringstream dropTable;
	dropTable << "DROP TABLE IF EXISTS \"" << model.schema() << "\".\"" << model.tableName() << "\";";
	for (const auto& f : model.fields()) {
		if ((f.contraintMask & (int)ConstraintType::AUTOINCREMENT) == 0) {
			continue;
		}
		dropTable << "DROP SEQUENCE IF EXISTS " << model.schema() << "." << model.tableName() << "_" << f.name;
		dropTable << "_seq;";
	}
	return dropTable.str();
}

std::string createUpdateStatement(const Model& model, BindParam* params) {
	const Fields& fields = model.fields();
	const std::string& tableName = model.tableName();
	std::stringstream update;
	update << "UPDATE \"" << model.schema() << "\".\"" << tableName << "\" SET (";
	for (auto i = fields.begin(); i != fields.end(); ++i) {
		if (i->isPrimaryKey()) {
			continue;
		}
		const Field& f = *i;
		if (i != fields.begin()) {
			update << ", ";
		}
		update << "\"" << f.name << "\"";
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
		update << "\"" << f.name << "\" = ";
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
	return core::string::format("DELETE FROM \"%s\".\"%s\"", table.schema().c_str(), table.tableName().c_str());
}

std::string createInsertStatement(const Model& model, BindParam* params) {
	std::stringstream insert;
	std::stringstream values;
	std::string autoincrement;
	std::string primaryKey;
	insert << "INSERT INTO \"" << model.schema() << "\".\"" << model.tableName() << "\" (";
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
		insert << "\"" << f.name << "\"";
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
		insert << " ON CONFLICT (\"" << primaryKey;
		insert << "\") DO UPDATE SET ";
		int fieldIndex = 0;
		for (const persistence::Field& f : model.fields()) {
			if (f.isPrimaryKey() || f.isAutoincrement()) {
				continue;
			}
			if (fieldIndex > 0) {
				insert << ", ";
			}
			insert << "\"" << f.name << "\"" << OperatorStrings[(int)f.updateOperator];
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
			return core::string::format("\"%s\"", fieldName.c_str());
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
			insert << "\"" << f.name << "\"";
			if (f.updateOperator != Operator::SET) {
				insert << " = \"" << model.tableName() << "\".\"" << f.name << "\"";
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
		insert << " RETURNING \"" << autoincrement << "\"";
	}
	return insert.str();
}

// https://www.postgresql.org/docs/current/static/functions-formatting.html
// https://www.postgresql.org/docs/current/static/functions-datetime.html
std::string createSelect(const Model& model, BindParam* params) {
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
		select << "\"" << f.name << "\"";
		if (f.type == FieldType::TIMESTAMP) {
			select << " AT TIME ZONE 'UTC') AS bigint) AS \"" << f.name << "\"";
		}
	}

	select << " FROM \"" << model.schema() << "\".\"" << tableName << "\"";
	return select.str();
}

std::string createWhere(const DBCondition& condition, int &parameterCount) {
	const std::string& conditionStr = condition.statement(parameterCount);
	return core::string::format(" WHERE %s", conditionStr.c_str());
}

std::string createOrderBy(const OrderBy& orderBy) {
	return core::string::format(" ORDER BY \"%s\" %s", orderBy.fieldname, OrderStrings[std::enum_value(orderBy.order)]);
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
