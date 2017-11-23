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
#include "MetainfoModel.h"
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
	if (model.isNull(field)) {
		core_assert(!field.isNotNull());
		ss << "NULL";
		return false;
	}
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

static void createCreateSequence(std::stringstream& stmt, const Model& table, const Field& field) {
	stmt << "CREATE SEQUENCE IF NOT EXISTS \"" << table.schema() << "\".\"" << table.tableName() << "_" << field.name;
	stmt << "_seq\" START " << table.autoIncrementStart() << ";";
}

static void createDropSequence(std::stringstream& stmt, const Model& table, const Field& field) {
	stmt << "DROP SEQUENCE IF EXISTS \"" << table.schema() << "\".\"" << table.tableName() << "_" << field.name << "_seq\";";
}

static void createDropSequence(std::stringstream& stmt, const Model& table, const db::MetainfoModel& field) {
	stmt << "DROP SEQUENCE IF EXISTS \"" << table.schema() << "\".\"" << table.tableName() << "_" << field.columnname() << "_seq\";";
}

template<class T>
static inline bool isSet(uint32_t mask, T value) {
	return (mask & (uint32_t)value) != 0u;
}

template<class T>
static inline bool changed(const db::MetainfoModel& schemaColumn, const Field& field, T value) {
	return isSet(schemaColumn.constraintmask(), value) != isSet(field.contraintMask, value);
}

template<class T>
static inline bool adds(const db::MetainfoModel& schemaColumn, const Field& field, T value) {
	return !isSet(schemaColumn.constraintmask(), value) && isSet(field.contraintMask, value);
}

template<class T>
static inline bool removes(const db::MetainfoModel& schemaColumn, const Field& field, T value) {
	return isSet(schemaColumn.constraintmask(), value) && !isSet(field.contraintMask, value);
}

static void createAlterTableAlterColumn(std::stringstream& stmt, bool add, const Model& table, const db::MetainfoModel& schemaColumn, const Field& field) {
	if (adds(schemaColumn, field, ConstraintType::AUTOINCREMENT)) {
		// TODO: pick max value and set to current
		createCreateSequence(stmt, table, field);
	}

	const char *action = add ? "ADD" : "ALTER";
	const std::string& base = core::string::format("ALTER TABLE \"%s\".\"%s\" %s COLUMN \"%s\"",
			table.schema().c_str(), table.tableName().c_str(), action, field.name.c_str());
	if (!add && adds(schemaColumn, field, ConstraintType::NOTNULL)) {
		stmt << base << " " << getDbType(field);
		if (!add) {
			stmt << " SET";
		}
		stmt << " NOT NULL;";
	} else if (!add && removes(schemaColumn, field, ConstraintType::NOTNULL)) {
		stmt << base << " DROP NOT NULL;";
	}
	if (add || toFieldType(schemaColumn.datatype()) != field.type || schemaColumn.maximumlength() != field.length) {
		stmt << base;
		if (!add) {
			stmt << " TYPE";
		}
		stmt << " " << getDbType(field);
		if (adds(schemaColumn, field, ConstraintType::NOTNULL)) {
			if (!add) {
				stmt << " SET";
			}
			stmt << " NOT NULL";
		}
		stmt << ";";
	}
	if (schemaColumn.columndefault() != field.defaultVal) {
		if (field.defaultVal.empty()) {
			if (!add) {
				stmt << base << " DROP DEFAULT;";
			}
		} else {
			stmt << base;
			if (!add) {
				stmt << " SET";
			}
			stmt << " DEFAULT " << field.defaultVal << ";";
		}
	} else {
		if (adds(schemaColumn, field, ConstraintType::AUTOINCREMENT)) {
			stmt << base;
			if (!add) {
				stmt << " SET";
			}
			stmt << " DEFAULT nextval('" << table.tableName() << "_" << field.name << "_seq'::regclass);";
		} else if (!add && removes(schemaColumn, field, ConstraintType::AUTOINCREMENT)) {
			stmt << base << " DROP DEFAULT;";
		}
	}
}

static void createAlterTableDropColumn(std::stringstream& stmt, const Model& table, const db::MetainfoModel& field) {
	stmt << "ALTER TABLE " << table.tableName() << " DROP COLUMN \"" << field.columnname() << "\";";
	if ((field.constraintmask() & (int)ConstraintType::AUTOINCREMENT) != 0) {
		createDropSequence(stmt, table, field);
	}
}

static void createAlterTableAddColumn(std::stringstream& stmt, const Model& table, const Field& field) {
	createAlterTableAlterColumn(stmt, true, table, db::MetainfoModel(), field);
}

static bool isDifferent(const db::MetainfoModel& schemaColumn, const Field& field) {
	if ((uint32_t)schemaColumn.constraintmask() != field.contraintMask) {
		Log::debug("%s differs in constraint mask", field.name.c_str());
#define C_CHECK(type) \
		if (adds(schemaColumn, field, ConstraintType::type)) { \
			Log::debug("Added " CORE_STRINGIFY(type)); \
		} else if (removes(schemaColumn, field, ConstraintType::type)) { \
			Log::debug("Removed " CORE_STRINGIFY(type)); \
		}

		C_CHECK(UNIQUE)
		C_CHECK(PRIMARYKEY)
		C_CHECK(AUTOINCREMENT)
		C_CHECK(NOTNULL)
		C_CHECK(INDEX)
		C_CHECK(FOREIGNKEY)

#undef C_CHECK

		return true;
	}
	if (schemaColumn.columndefault() != field.defaultVal) {
		Log::debug("%s differs in default values ('%s' vs '%s')",
				field.name.c_str(), schemaColumn.columndefault().c_str(), field.defaultVal.c_str());
		return true;
	}
	if (toFieldType(schemaColumn.datatype()) != field.type) {
		Log::debug("%s differs in types ('%s' vs '%s')",
				field.name.c_str(), schemaColumn.datatype().c_str(), toFieldType(field.type));
		return true;
	}
	if (schemaColumn.maximumlength() != field.length) {
		Log::debug("%s differs in length ('%i' vs '%i')",
				field.name.c_str(), schemaColumn.maximumlength(), field.length);
		return true;
	}
	return false;
}

static inline void uniqueConstraintName(std::stringstream& stmt, const Model& table, const std::set<std::string>& uniqueKey) {
	stmt << table.tableName() << "_" << core::string::join(uniqueKey.begin(), uniqueKey.end(), "_");
}

static inline void foreignKeyConstraintName(std::stringstream& stmt, const Model& table, const ForeignKey& foreignKey) {
	stmt << table.tableName() << "_" << foreignKey.table << "_" << foreignKey.field;
}

std::string createAlterTableStatement(const std::vector<db::MetainfoModel>& columns, const Model& table, bool useForeignKeys) {
	std::stringstream stmt;

	stmt << "CREATE SCHEMA IF NOT EXISTS \"" << table.schema() << "\";";

	std::unordered_map<std::string, const db::MetainfoModel*> map;
	map.reserve(columns.size());
	for (const auto& c : columns) {
		map.insert(std::make_pair(c.columnname(), &c));
		Log::debug("Handle expected column '%s' in table '%s'", c.columnname().c_str(), c.tablename().c_str());
		const Field& f = table.getField(c.columnname());
		if (f.name.empty()) {
			Log::debug("Drop column '%s' from table '%s' - no longer in the model",
					c.columnname().c_str(), c.tablename().c_str());
			// the field is not known in the current table structure - but it's known in the
			// database, so get rid of the column
			createAlterTableDropColumn(stmt, table, c);
		} else {
			Log::debug("Column '%s' in table '%s' still exists - check for needed updates",
					c.columnname().c_str(), c.tablename().c_str());
		}
	}
	bool uniqueConstraintDiffers = false;
	bool foreignKeysDiffers = false;
	for (const auto& f : table.fields()) {
		auto i = map.find(f.name);
		if (i != map.end()) {
			// the field already exists, but it might be different from what we expect
			// to find in the database
			if (isDifferent(*i->second, f)) {
				if (changed(*i->second, f, ConstraintType::UNIQUE)) {
					uniqueConstraintDiffers = true;
				}
				if (changed(*i->second, f, ConstraintType::FOREIGNKEY)) {
					foreignKeysDiffers = true;
				}
				Log::debug("Column '%s' in table '%s' differs - update it",
						f.name.c_str(), table.tableName().c_str());
				createAlterTableAlterColumn(stmt, false, table, *i->second, f);
			}
			continue;
		}
		Log::debug("Column '%s' in table '%s' doesn't exist yet - create it",
				f.name.c_str(), table.tableName().c_str());
		// a new field that is not yet known in the database schema was added to the model
		// now just create the new column.
		createAlterTableAddColumn(stmt, table, f);
		if (f.isUnique()) {
			uniqueConstraintDiffers = true;
		}
		if (f.isForeignKey()) {
			foreignKeysDiffers = true;
		}

		if (useForeignKeys && foreignKeysDiffers) {
			for (const auto& foreignKey : table.foreignKeys()) {
				stmt << "ALTER TABLE \"" << table.schema() << "\".\"" << table.tableName() << "\" DROP CONSTRAINT IF EXISTS ";
				foreignKeyConstraintName(stmt, table, foreignKey.second);
				stmt << ";";
				stmt << "ALTER TABLE \"" << table.schema() << "\".\"" << table.tableName() << "\" ADD CONSTRAINT ";
				foreignKeyConstraintName(stmt, table, foreignKey.second);
				stmt << " FOREIGN KEY(\"" << foreignKey.first << "\") REFERENCES \"";
				stmt << foreignKey.second.table << "\"(\"" << foreignKey.second.field;
				stmt << "\") MATCH SIMPLE ON UPDATE NO ACTION ON DELETE NO ACTION";
			}
		}
	}

	if (uniqueConstraintDiffers) {
		for (const auto& uniqueKey : table.uniqueKeys()) {
			stmt << "ALTER TABLE \"" << table.schema() << "\".\"" << table.tableName() << "\" DROP CONSTRAINT IF EXISTS ";
			uniqueConstraintName(stmt, table, uniqueKey);
			stmt << ";";
			stmt << "ALTER TABLE \"" << table.schema() << "\".\"" << table.tableName() << "\" ADD CONSTRAINT ";
			uniqueConstraintName(stmt, table, uniqueKey);
			stmt << " UNIQUE(";
			bool firstUniqueKey = true;
			for (const std::string& fieldName : uniqueKey) {
				if (!firstUniqueKey) {
					stmt << ", ";
				}
				stmt << "\"" << fieldName << "\"";
				firstUniqueKey = false;
			}
			stmt << ");";
		}
	}

	// TODO: index support
	// TODO: multiple-field-pk

	return stmt.str();
}

std::string createCreateTableStatement(const Model& table, bool useForeignKeys) {
	std::stringstream createTable;

	createTable << "CREATE SCHEMA IF NOT EXISTS \"" << table.schema() << "\";";

	for (const auto& f : table.fields()) {
		if ((f.contraintMask & (int)ConstraintType::AUTOINCREMENT) == 0) {
			continue;
		}
		createCreateSequence(createTable, table, f);
	}

	createTable << "CREATE TABLE IF NOT EXISTS \"" << table.schema() << "\".\"" << table.tableName() << "\" (";
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
			uniqueConstraintName(createTable, table, uniqueKey);
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
			foreignKeyConstraintName(createTable, table, foreignKey.second);
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
		createTable << "CREATE INDEX IF NOT EXISTS \"" << table.schema() << "\"." << table.tableName() << "_" << f.name << " ON \"";
		createTable << table.tableName() << "\" USING btree (\"" << f.name << "\");";
	}

	return createTable.str();
}

std::string createTruncateTableStatement(const Model& model) {
	return core::string::format("TRUNCATE TABLE \"%s\".\"%s\";", model.schema().c_str(), model.tableName().c_str());
}

std::string createDropTableStatement(const Model& model) {
	std::stringstream stmt;
	stmt << "DROP TABLE IF EXISTS \"" << model.schema() << "\".\"" << model.tableName() << "\";";
	for (const auto& f : model.fields()) {
		if ((f.contraintMask & (int)ConstraintType::AUTOINCREMENT) == 0) {
			continue;
		}
		createDropSequence(stmt, model, f);
	}

	return stmt.str();
}

static void createWhereStatementsForKeys(std::stringstream& stmt, int index, const Model& model, BindParam* params) {
	int where = 0;
	const Fields& fields = model.fields();
	for (auto i = fields.begin(); i != fields.end(); ++i) {
		const Field& f = *i;
		if (!model.isValid(f)) {
			continue;
		}
		if (!f.isPrimaryKey()) {
			continue;
		}
		if (where > 0) {
			stmt << " AND ";
		} else {
			stmt << " WHERE ";
		}
		++where;
		stmt << "\"" << f.name << "\"";
		if (model.isNull(f)) {
			stmt << " IS ";
		} else {
			stmt << " = ";
		}
		if (placeholder(model, f, stmt, index)) {
			++index;
			if (params != nullptr) {
				params->push(model, f);
			}
		}
	}
}

std::string createUpdateStatement(const Model& model, BindParam* params) {
	const Fields& fields = model.fields();
	const std::string& tableName = model.tableName();
	std::stringstream update;
	update << "UPDATE \"" << model.schema() << "\".\"" << tableName << "\" SET ";
	int updateFields = 0;
	int index = 1;
	for (auto i = fields.begin(); i != fields.end(); ++i) {
		const Field& f = *i;
		if (!model.isValid(f)) {
			continue;
		}
		if (f.isPrimaryKey()) {
			continue;
		}
		if (updateFields > 0) {
			update << ", ";
		}
		update << "\"" << f.name << "\" = ";
		if (placeholder(model, f, update, index)) {
			++index;
			if (params != nullptr) {
				params->push(model, f);
			}
		}
		++updateFields;
	}

	createWhereStatementsForKeys(update, index, model, params);

	return update.str();
}

std::string createDeleteStatement(const Model& table, BindParam* params) {
	std::stringstream stmt;
	stmt << "DELETE FROM \"" << table.schema() << "\".\"" << table.tableName() << "\"";
	createWhereStatementsForKeys(stmt, 1, table, params);
	return stmt.str();
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
		if (f.isAutoincrement()) {
			autoincrement = f.name;
		}
		if (!model.isValid(f)) {
			continue;
		}
		if (f.isPrimaryKey()) {
			primaryKey = f.name;
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
	if (model.primaryKeys() == 1 && !primaryKey.empty()) {
		insert << " ON CONFLICT (\"" << primaryKey;
		insert << "\") DO UPDATE SET ";
		int fieldIndex = 0;
		for (const persistence::Field& f : model.fields()) {
			if (!model.isValid(f)) {
				continue;
			}
			if (f.isPrimaryKey() || f.isAutoincrement()) {
				continue;
			}
			if (fieldIndex > 0) {
				insert << ", ";
			}
			insert << "\"" << f.name << "\" = ";
			if (f.updateOperator != Operator::SET) {
				insert << "\"" << model.schema() << "\".\"" << model.tableName() << "\".\"" << f.name << "\"";
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
			insert << "\"" << f.name << "\" = ";
			if (f.updateOperator != Operator::SET) {
				insert << "\"" << model.schema() << "\".\"" << model.tableName() << "\".\"" << f.name << "\"";
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
	insert << ";";
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
	createWhereStatementsForKeys(select, 1, model, params);
	return select.str();
}

std::string createWhere(const DBCondition& condition, int &parameterCount) {
	const bool needWhere = parameterCount == 0;
	const std::string& conditionStr = condition.statement(parameterCount);
	if (conditionStr.empty()) {
		return conditionStr;
	}
	return core::string::format("%s %s", (needWhere ? " WHERE" : ""), conditionStr.c_str());
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
