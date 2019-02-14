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

static inline void createSchemaIdentifier(std::stringstream& stmt, const Model& table) {
	stmt << "\"" << table.schema() << "\"";
}

static inline void createIndexIdentifier(std::stringstream& stmt, const Model& table, const std::string& field) {
	stmt << "\"" << table.tableName() << "_" << field << "\"";
}

static inline void createTableIdentifier(std::stringstream& stmt, const Model& table) {
	createSchemaIdentifier(stmt, table);
	stmt << ".\"" << table.tableName() << "\"";
}

static inline void createSequenceIdentifier(std::stringstream& stmt, const Model& table, const std::string& field) {
	createSchemaIdentifier(stmt, table);
	stmt << ".\"" << table.tableName() << "_" << field << "_seq\"";
}

static inline bool placeholder(const Model& table, const Field& field, std::stringstream& ss, int count, bool select) {
	if (table.isNull(field)) {
		core_assert(!field.isNotNull());
		ss << "NULL";
		return false;
	}
	if (field.type == FieldType::PASSWORD) {
		ss << "crypt($" << count << ", ";
		if (select) {
			ss << field.name;
		} else {
			ss << "gen_salt('bf', 8)";
		}
		ss << ")";
	} else if (field.type == FieldType::TIMESTAMP) {
		const Timestamp& ts = table.getValue<Timestamp>(field);
		if (ts.isNow()) {
			ss << "NOW() AT TIME ZONE 'UTC'";
			return false;
		}
		ss << "to_timestamp($" << count << ") AT TIME ZONE 'UTC'";
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
		return "TIMESTAMP WITHOUT TIME ZONE";
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
		return "SMALLINT";
	case FieldType::STRING:
	case FieldType::PASSWORD:
	case FieldType::MAX:
		break;
	}
	return "";
}

static void createCreateSequence(std::stringstream& stmt, const Model& table, const Field& field) {
	stmt << "CREATE SEQUENCE IF NOT EXISTS ";
	createSequenceIdentifier(stmt, table, field.name);
	stmt << " START " << table.autoIncrementStart() << ";";
}

static void createDropSequence(std::stringstream& stmt, const Model& table, const Field& field) {
	stmt << "DROP SEQUENCE IF EXISTS ";
	createSequenceIdentifier(stmt, table, field.name);
	stmt << ";";
}

static void createDropSequence(std::stringstream& stmt, const Model& table, const db::MetainfoModel& field) {
	stmt << "DROP SEQUENCE IF EXISTS ";
	createSequenceIdentifier(stmt, table, field.columnname());
	stmt << ";";
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

static inline void uniqueConstraintName(std::stringstream& stmt, const Model& table, const std::set<std::string>& uniqueKey) {
	stmt << table.tableName() << "_" << core::string::join(uniqueKey.begin(), uniqueKey.end(), "_") << "_unique";
}

static inline void foreignKeyConstraintName(std::stringstream& stmt, const Model& table, const ForeignKey& foreignKey) {
	stmt << table.tableName() << "_" << foreignKey.table << "_" << foreignKey.field << "_fk";
}

static void createAlterTableAlterColumn(std::stringstream& stmt, bool add, const Model& table, const db::MetainfoModel& schemaColumn, const Field& field) {
	if (removes(schemaColumn, field, ConstraintType::INDEX)) {
		stmt << "DROP INDEX IF EXISTS ";
		createIndexIdentifier(stmt, table, field.name);
		stmt << ";";
	}

	if (adds(schemaColumn, field, ConstraintType::AUTOINCREMENT)) {
		// TODO: pick max value and set to current
		createCreateSequence(stmt, table, field);
	}

	const char *action = add ? "ADD" : "ALTER";
	const std::string& base = core::string::format("ALTER TABLE \"%s\".\"%s\" %s COLUMN \"%s\"",
			table.schema(), table.tableName(), action, field.name.c_str());
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
			stmt << " DEFAULT nextval('";
			createSequenceIdentifier(stmt, table, field.name);
			stmt << "'::regclass);";
		} else if (!add && removes(schemaColumn, field, ConstraintType::AUTOINCREMENT)) {
			stmt << base << " DROP DEFAULT;";
		}
	}

	if (adds(schemaColumn, field, ConstraintType::INDEX)) {
		stmt << "CREATE INDEX IF NOT EXISTS ";
		createIndexIdentifier(stmt, table, field.name);
		stmt << " ON ";
		createTableIdentifier(stmt, table);
		stmt << " USING btree (\"" << field.name << "\");";
	}
}

static void createAlterTableDropColumn(std::stringstream& stmt, const Model& table, const db::MetainfoModel& field) {
	stmt << "ALTER TABLE ";
	createTableIdentifier(stmt, table);
	stmt << " DROP COLUMN \"" << field.columnname() << "\" CASCADE;";
	if ((field.constraintmask() & (int)ConstraintType::AUTOINCREMENT) != 0) {
		createDropSequence(stmt, table, field);
	}
}

static void createAlterTableAddColumn(std::stringstream& stmt, const Model& table, const Field& field) {
	createAlterTableAlterColumn(stmt, true, table, db::MetainfoModel(), field);
}

static bool isDifferent(const db::MetainfoModel& schemaColumn, const Field& field) {
	if ((uint32_t)schemaColumn.constraintmask() != field.contraintMask) {
		Log::debug("  - %s differs in constraint mask", field.name.c_str());
#define C_CHECK(type) \
		if (adds(schemaColumn, field, ConstraintType::type)) { \
			Log::debug("  - Added " CORE_STRINGIFY(type) " to field %s", field.name.c_str()); \
		} else if (removes(schemaColumn, field, ConstraintType::type)) { \
			Log::debug("  - Removed " CORE_STRINGIFY(type) " from field %s", field.name.c_str()); \
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
		Log::debug("  - %s differs in default values ('%s' vs '%s')",
				field.name.c_str(), schemaColumn.columndefault().c_str(), field.defaultVal.c_str());
		return true;
	}
	if (toFieldType(schemaColumn.datatype()) != field.type) {
		Log::debug("  - %s differs in types ('%s' vs '%s')",
				field.name.c_str(), schemaColumn.datatype().c_str(), toFieldType(field.type));
		return true;
	}
	if (schemaColumn.maximumlength() != field.length) {
		Log::debug("  - %s differs in length ('%i' vs '%i')",
				field.name.c_str(), schemaColumn.maximumlength(), field.length);
		return true;
	}
	return false;
}

std::string createTableExistsStatement(const Model& model, BindParam* params) {
	if (params != nullptr) {
		const int indexSchema = params->add();
		params->valueBuffers.emplace_back(model.schema());
		params->values[indexSchema] = params->valueBuffers.back().c_str();

		const int indexTable = params->add();
		params->valueBuffers.emplace_back(model.tableName());
		params->values[indexTable] = params->valueBuffers.back().c_str();
	}
	return R"(SELECT EXISTS (SELECT 1 FROM "pg_tables" WHERE "schemaname" = $1 AND "tablename" = $2);)";
}

std::string createAlterTableStatement(const std::vector<db::MetainfoModel>& columns, const Model& table, bool useForeignKeys) {
	std::stringstream stmt;

	// TODO: allow to move into new schema?
	stmt << "CREATE SCHEMA IF NOT EXISTS ";
	createSchemaIdentifier(stmt, table);
	stmt << ";";

	std::unordered_map<std::string, const db::MetainfoModel*> map;
	map.reserve(columns.size());
	for (const auto& c : columns) {
		map.insert(std::make_pair(c.columnname(), &c));
		Log::debug("# Column '%s' in table '%s'", c.columnname().c_str(), c.tablename().c_str());
		const Field& f = table.getField(c.columnname());
		if (f.name.empty()) {
			Log::debug("- Column '%s' in table '%s' - no longer in the model, drop it",
					c.columnname().c_str(), c.tablename().c_str());
			// the field is not known in the current table structure - but it's known in the
			// database, so get rid of the column
			createAlterTableDropColumn(stmt, table, c);
		} else {
			Log::debug("- Column '%s' in table '%s' still exists - check for needed updates",
					c.columnname().c_str(), c.tablename().c_str());
		}
	}
	bool uniqueConstraintDiffers = false;
	bool foreignKeysDiffers = false;
	for (const auto& f : table.fields()) {
		Log::debug("# Column '%s' in table '%s'", f.name.c_str(), table.tableName());
		auto i = map.find(f.name);
		if (i != map.end()) {
			Log::debug("- Column '%s' in table '%s' already exists", f.name.c_str(), table.tableName());
			// the field already exists, but it might be different from what we expect
			// to find in the database
			if (isDifferent(*i->second, f)) {
				if (changed(*i->second, f, ConstraintType::UNIQUE)) {
					uniqueConstraintDiffers = true;
				}
				if (changed(*i->second, f, ConstraintType::FOREIGNKEY)) {
					foreignKeysDiffers = true;
				}
				Log::debug("- Column '%s' in table '%s' differs - update it",
						f.name.c_str(), table.tableName());
				createAlterTableAlterColumn(stmt, false, table, *i->second, f);
			}
			continue;
		}
		Log::debug("- Column '%s' in table '%s' doesn't exist yet - create it",
				f.name.c_str(), table.tableName());
		// a new field that is not yet known in the database schema was added to the model
		// now just create the new column.
		createAlterTableAddColumn(stmt, table, f);
		if (f.isUnique()) {
			uniqueConstraintDiffers = true;
		}
		if (f.isForeignKey()) {
			foreignKeysDiffers = true;
		}
	}

	if (useForeignKeys && foreignKeysDiffers) {
		for (const auto& foreignKey : table.foreignKeys()) {
			stmt << "ALTER TABLE ";
			createTableIdentifier(stmt, table);
			stmt << " DROP CONSTRAINT IF EXISTS ";
			foreignKeyConstraintName(stmt, table, foreignKey.second);
			stmt << ";";
			stmt << "ALTER TABLE ";
			createTableIdentifier(stmt, table);
			stmt << " ADD CONSTRAINT ";
			foreignKeyConstraintName(stmt, table, foreignKey.second);
			stmt << " FOREIGN KEY(\"" << foreignKey.first << "\") REFERENCES \"";
			stmt << foreignKey.second.table << "\"(\"" << foreignKey.second.field;
			stmt << "\") MATCH SIMPLE ON UPDATE NO ACTION ON DELETE NO ACTION";
		}
	}

	if (uniqueConstraintDiffers) {
		for (const auto& uniqueKey : table.uniqueKeys()) {
			stmt << "ALTER TABLE ";
			createTableIdentifier(stmt, table);
			stmt << " DROP CONSTRAINT IF EXISTS ";
			uniqueConstraintName(stmt, table, uniqueKey);
			stmt << ";";
			stmt << "ALTER TABLE ";
			createTableIdentifier(stmt, table);
			stmt << " ADD CONSTRAINT ";
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

	// TODO: multiple-field-pk

	return stmt.str();
}

std::string createCreateTableStatement(const Model& table, bool useForeignKeys) {
	std::stringstream stmt;

	stmt << "CREATE SCHEMA IF NOT EXISTS ";
	createSchemaIdentifier(stmt, table);
	stmt << ";";

	for (const auto& f : table.fields()) {
		if ((f.contraintMask & (int)ConstraintType::AUTOINCREMENT) == 0) {
			continue;
		}
		createCreateSequence(stmt, table, f);
	}

	stmt << "CREATE TABLE IF NOT EXISTS ";
	createTableIdentifier(stmt, table);
	stmt << " (";
	bool firstField = true;
	for (const auto& f : table.fields()) {
		if (!firstField) {
			stmt << ", ";
		}
		stmt << "\"" << f.name << "\"";
		const std::string& dbType = getDbType(f);
		if (!dbType.empty()) {
			stmt << " " << dbType;
		}
		const std::string& flags = getDbFlags(table.tableName(), table.primaryKeyFields(), table.constraints(), f);
		if (!flags.empty()) {
			stmt << " " << flags;
		}
		firstField = false;
	}

	if (!table.uniqueKeys().empty()) {
		for (const auto& uniqueKey : table.uniqueKeys()) {
			stmt << ", CONSTRAINT ";
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
			stmt << ")";
		}
	}

	if (table.primaryKeyFields() > 1) {
		stmt << ", PRIMARY KEY(";
		bool firstPrimaryKey = true;
		for (const auto& f : table.fields()) {
			if (!f.isPrimaryKey()) {
				continue;
			}
			if (!firstPrimaryKey) {
				stmt << ", ";
			}
			stmt << "\"" << f.name << "\"";
			firstPrimaryKey = false;
		}
		stmt << ")";
	}

	if (useForeignKeys) {
		for (const auto& foreignKey : table.foreignKeys()) {
			stmt << ", CONSTRAINT ";
			foreignKeyConstraintName(stmt, table, foreignKey.second);
			stmt << " FOREIGN KEY(\"" << foreignKey.first << "\") REFERENCES \"";
			stmt << foreignKey.second.table << "\"(\"" << foreignKey.second.field;
			stmt << "\") MATCH SIMPLE ON UPDATE NO ACTION ON DELETE NO ACTION";
		}
	}

	stmt << ");";

	for (const auto& f : table.fields()) {
		if (!f.isIndex()) {
			continue;
		}
		stmt << "CREATE INDEX IF NOT EXISTS ";
		createIndexIdentifier(stmt, table, f.name);
		stmt << " ON ";
		createTableIdentifier(stmt, table);
		stmt << " USING btree (\"" << f.name << "\");";
	}

	return stmt.str();
}

std::string createTruncateTableStatement(const Model& model) {
	return core::string::format("TRUNCATE TABLE \"%s\".\"%s\";", model.schema(), model.tableName());
}

std::string createDropTableStatement(const Model& model) {
	std::stringstream stmt;
	stmt << "DROP TABLE IF EXISTS ";
	createTableIdentifier(stmt, model);
	stmt << ";";
	for (const auto& f : model.fields()) {
		if ((f.contraintMask & (int)ConstraintType::AUTOINCREMENT) == 0) {
			continue;
		}
		createDropSequence(stmt, model, f);
	}

	return stmt.str();
}

static void createWhereStatementsForKeys(std::stringstream& stmt, int& index, const Model& model, BindParam* params) {
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
		if (placeholder(model, f, stmt, index, true)) {
			++index;
			if (params != nullptr) {
				params->push(model, f);
			}
		}
	}
}

std::string createUpdateStatement(const Model& table, BindParam* params, int* parameterCount) {
	std::stringstream stmt;
	stmt << "UPDATE ";
	createTableIdentifier(stmt, table);
	stmt << " SET ";
	int updateFields = 0;
	int index = 1;
	const Fields& fields = table.fields();
	for (auto i = fields.begin(); i != fields.end(); ++i) {
		const Field& f = *i;
		if (!table.isValid(f)) {
			continue;
		}
		if (f.isPrimaryKey()) {
			continue;
		}
		if (updateFields > 0) {
			stmt << ", ";
		}
		stmt << "\"" << f.name << "\" = ";
		if (placeholder(table, f, stmt, index, false)) {
			++index;
			if (params != nullptr) {
				params->push(table, f);
			}
		}
		++updateFields;
	}

	createWhereStatementsForKeys(stmt, index, table, params);

	if (parameterCount != nullptr) {
		*parameterCount = index - 1;
	}

	return stmt.str();
}

std::string createDeleteStatement(const Model& table, BindParam* params) {
	std::stringstream stmt;
	stmt << "DELETE FROM ";
	createTableIdentifier(stmt, table);
	int index = 1;
	createWhereStatementsForKeys(stmt, index, table, params);
	return stmt.str();
}

std::string createInsertBaseStatement(const Model& table, bool& primaryKeyIncluded) {
	std::stringstream stmt;
	stmt << "INSERT INTO ";
	createTableIdentifier(stmt, table);
	stmt << " (";
	int inserted = 0;
	for (const persistence::Field& f : table.fields()) {
		if (!table.isValid(f)) {
			continue;
		}
		if (inserted > 0) {
			stmt << ", ";
		}
		if (f.isPrimaryKey()) {
			primaryKeyIncluded = true;
		}
		stmt << "\"" << f.name << "\"";
		++inserted;
	}

	stmt << ")";

	return stmt.str();
}

std::string createInsertValuesStatement(const Model& table, BindParam* params, int& insertValueIndex) {
	std::stringstream stmt;
	stmt << "(";
	int inserted = 0;
	for (const persistence::Field& f : table.fields()) {
		if (!table.isValid(f)) {
			continue;
		}
		if (inserted > 0) {
			stmt << ", ";
		}
		++inserted;
		if (placeholder(table, f, stmt, insertValueIndex, false)) {
			++insertValueIndex;
			if (params != nullptr) {
				params->push(table, f);
			}
		}
	}
	stmt << ")";
	return stmt.str();
}

void createUpsertStatement(const Model& table, std::stringstream& stmt, bool primaryKeyIncluded, int insertValueIndex) {
	if (primaryKeyIncluded && !table.primaryKeys().empty()) {
		stmt << " ON CONFLICT (";
		auto i = table.primaryKeys().begin();
		stmt << "\"" << *i << "\"";
		for (++i; i != table.primaryKeys().end(); ++i) {
			stmt << ", \"" << *i << "\"";
		}
		stmt << ") DO ";
		if (insertValueIndex <= (int)table.primaryKeys().size()) {
			stmt << "NOTHING";
		} else {
			stmt << "UPDATE SET ";
			int fieldIndex = 0;
			for (const persistence::Field& f : table.fields()) {
				if (!table.isValid(f)) {
					continue;
				}
				if (f.isPrimaryKey() || f.isAutoincrement()) {
					continue;
				}
				if (fieldIndex > 0) {
					stmt << ", ";
				}
				stmt << "\"" << f.name << "\" = ";
				if (f.updateOperator != Operator::SET) {
					stmt << "\"" << table.schema() << "\".\"" << table.tableName() << "\".\"" << f.name << "\"";
					stmt << OperatorStrings[(int)f.updateOperator];
				}
				stmt << "EXCLUDED.\"" << f.name << "\"";
				++fieldIndex;
			}
		}
		// right now the on conflict syntax doesn't permit to repeat the clause.
		// https://www.postgresql.org/docs/current/static/sql-insert.html
		return;
	}
	const UniqueKeys uniqueKeys = table.uniqueKeys();
	for (const auto& set : uniqueKeys) {
		for (const persistence::Field& f : table.fields()) {
			if (!table.isValid(f)) {
				continue;
			}
			if (f.isPrimaryKey() || f.isAutoincrement()) {
				continue;
			}
			if (set.find(f.name) == set.end()) {
				continue;
			}
			stmt << " ON CONFLICT ON CONSTRAINT \"";
			uniqueConstraintName(stmt, table, set);
			stmt << "\" DO ";
			if (insertValueIndex == 1) {
				stmt << "NOTHING";
			} else {
				stmt << "UPDATE SET ";
				int fieldIndex = 0;
				for (const persistence::Field& tblField : table.fields()) {
					if (!table.isValid(tblField)) {
						continue;
					}
					if (tblField.isPrimaryKey() || tblField.isAutoincrement()) {
						continue;
					}
					if (set.find(tblField.name) != set.end()) {
						continue;
					}
					if (fieldIndex > 0) {
						stmt << ", ";
					}
					stmt << "\"" << tblField.name << "\" = ";
					if (tblField.updateOperator != Operator::SET) {
						stmt << "\"" << table.schema() << "\".\"" << table.tableName() << "\".\"" << tblField.name << "\"";
						stmt << OperatorStrings[(int)tblField.updateOperator];
					}
					stmt << "EXCLUDED.\"" << tblField.name << "\"";
					++fieldIndex;
				}
			}
			// right now the on conflict syntax doesn't permit to repeat the clause.
			// https://www.postgresql.org/docs/current/static/sql-insert.html
			return;
		}
	}
}

std::string createInsertStatement(const std::vector<const Model*>& tables, BindParam* params, int* parameterCount) {
	const Model& table = *tables.front();

	bool primaryKeyIncluded = false;
	std::stringstream stmt;
	stmt << createInsertBaseStatement(table, primaryKeyIncluded);
	stmt << " VALUES ";
	int insertValueIndex = 1;

	auto tableIter = tables.begin();
	stmt << createInsertValuesStatement(**tableIter, params, insertValueIndex);
	for (++tableIter; tableIter != tables.end(); ++tableIter) {
		stmt << "," << createInsertValuesStatement(**tableIter, params, insertValueIndex);
	}

	createUpsertStatement(table, stmt, primaryKeyIncluded, insertValueIndex - 1);

	const char* autoIncField = table.autoIncrementField();
	if (autoIncField != nullptr) {
		stmt << " RETURNING \"" << autoIncField << "\"";
	}
	stmt << ";";

	if (parameterCount != nullptr) {
		*parameterCount = insertValueIndex - 1;
	}

	return stmt.str();
}

std::string createInsertStatement(const Model& table, BindParam* params, int* parameterCount) {
	return createInsertStatement({&table}, params, parameterCount);
}

// https://www.postgresql.org/docs/current/static/functions-formatting.html
// https://www.postgresql.org/docs/current/static/functions-datetime.html
std::string createSelect(const Model& table, BindParam* params) {
	const Fields& fields = table.fields();
	std::stringstream stmt;
	stmt << "SELECT ";
	int select = 0;
	for (auto i = fields.begin(); i != fields.end(); ++i) {
		const Field& f = *i;
		if (f.type == FieldType::PASSWORD) {
			// don't load passwords into memory
			continue;
		}
		if (select > 0) {
			stmt << ", ";
		}
		++select;
		if (f.type == FieldType::TIMESTAMP) {
			stmt << "CAST(EXTRACT(EPOCH FROM ";
		}
		stmt << "\"" << f.name << "\"";
		if (f.type == FieldType::TIMESTAMP) {
			stmt << " AT TIME ZONE 'UTC') AS bigint) AS \"" << f.name << "\"";
		}
	}

	core_assert_always(select > 0);
	stmt << " FROM ";
	createTableIdentifier(stmt, table);
	int index = 1;
	createWhereStatementsForKeys(stmt, index, table, params);
	return stmt.str();
}

/**
 * @param[in] condition The condition to generate the where clause for
 * @param[in,out] parameterCount The amount of already existing where clauses due
 * to primary keys that are not part of the condition
 */
std::string createWhere(const DBCondition& condition, int &parameterCount) {
	const bool needWhere = parameterCount == 0;
	const std::string& conditionStr = condition.statement(parameterCount);
	if (conditionStr.empty()) {
		return conditionStr;
	}
	return core::string::format("%s %s", (needWhere ? " WHERE" : " AND"), conditionStr.c_str());
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
