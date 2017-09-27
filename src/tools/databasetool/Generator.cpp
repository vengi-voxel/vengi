#include "Generator.h"
#include "Util.h"
#include "Mapping.h"
#include "Table.h"
#include "persistence/DBHandler.h"
#include "core/String.h"

namespace databasetool {

static const char *quote = "\\\"";
#define NAMESPACE "db"

struct Namespace {
	const Table& _table;
	std::stringstream& _src;

	Namespace(const Table& table, std::stringstream& src) : _table(table), _src(src) {
		if (!table.namespaceSrc.empty()) {
			src << "namespace " << table.namespaceSrc << " {\n\n";
		}
		src << "namespace " NAMESPACE " {\n\n";
	}
	~Namespace() {
		_src << "typedef std::shared_ptr<" << _table.classname << "> " << _table.classname << "Ptr;\n\n";
		_src << "} // namespace " NAMESPACE "\n\n";
		if (!_table.namespaceSrc.empty()) {
			_src << "} // namespace " << _table.namespaceSrc << "\n\n";
		}
	}
};

struct Class {
	const Table& _table;
	std::stringstream& _src;

	Class(const Table& table, std::stringstream& src) : _table(table), _src(src) {
		src << "class " << table.classname << " : public persistence::Model {\n";
		src << "private:\n";
		src << "\tusing Super = persistence::Model;\n";
	}

	~Class() {
		_src << "}; // class " << _table.classname << "\n\n";
	}
};

struct MembersStruct {
	static const char *structName() {
		return "Members";
	}

	static const char *varName() {
		return "_m";
	}

	static std::string nullFieldName(const persistence::Model::Field& f) {
		return "_isNull_" + f.name;
	}
};

static void createMembersStruct(const Table& table, std::stringstream& src) {
	src << "\tstruct " << MembersStruct::structName() << " {\n";
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		src << "\t\t";
		src << getCPPType(f.type, false);
		src << " _" << f.name;
		if (needsInitCPP(f.type)) {
			src << " = " << getCPPInit(f.type, false);
		}
		src << ";\n";
		if (isPointer(f)) {
			src << "\t\tbool " << MembersStruct::nullFieldName(f) << " = false;\n";
		}
	}
	src << "\t};\n";
	src << "\tMembers " << MembersStruct::varName() << ";\n";
}

static void createMetaInformation(const Table& table, std::stringstream& src) {
	src << "\tstatic constexpr int primaryKeys = " << table.primaryKeys << ";\n\n";
	src << "\tstatic const std::vector<Constraint>& constraints() {\n";
	src << "\t\tstatic const std::vector<Constraint> _constraints {\n";
	for (auto i = table.constraints.begin(); i != table.constraints.end(); ++i) {
		const persistence::Model::Constraint& c = i->second;
		if (i != table.constraints.begin()) {
			src << ",\n";
		}
		src << "\t\t\t{{\"";
		src << core::string::join(c.fields.begin(), c.fields.end(), "\",\"");
		src << "\"}, " << c.types << "}";
	}
	if (!table.constraints.empty()) {
		src << "\n";
	}
	src << "\t\t};\n";
	src << "\t\treturn _constraints;\n";
	src << "\t};\n\n";

	src << "\tstatic const std::array<std::vector<std::string>, " << table.uniqueKeys.size() << ">& uniqueKeys() {\n";
	src << "\t\tstatic const std::array<std::vector<std::string>, " << table.uniqueKeys.size() << "> _uniquekeys {\n";
	for (const auto& uniqueKey : table.uniqueKeys) {
		src << "\t\t\tstd::vector<std::string>{\"";
		src << core::string::join(uniqueKey.begin(), uniqueKey.end(), "\", \"");
		src << "\"},\n"; // TODO: remove last ,
	}
	src << "\t\t};\n";
	src << "\t\treturn _uniquekeys;\n";
	src << "\t};\n\n";
}

void createConstructor(const Table& table, std::stringstream& src) {
	src << "\t" << table.classname << "(";
	src << ") : Super(\"" << table.name << "\") {\n";
	src << "\t\t_membersPointer = (uint8_t*)&" << MembersStruct::varName() << ";\n";
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		src << "\t\t_fields.push_back(Field{";
		src << "\"" << f.name << "\"";
		src << ", FieldType::" << FieldTypeNames[std::enum_value(f.type)];
		src << ", " << f.contraintMask;
		src << ", \"" << f.defaultVal << "\"";
		src << ", " << f.length;
		src << ", offsetof(";
		src << MembersStruct::structName() << ", _" << f.name << ")";
		if (isPointer(f)) {
			src << ", offsetof(";
			src << MembersStruct::structName() << ", " << MembersStruct::nullFieldName(f) << ")";
		} else {
			src << ", -1";
		}
		src << "});\n";
	}
	src << "\t}\n\n";
}

static void createSelectStatement(const Table& table, std::stringstream& src) {
	if (table.primaryKeys <= 0) {
		return;
	}
	std::stringstream loadNonPk;
	loadNonPk << "\t\tstd::stringstream __load_;\n\t\tint __count_ = 1;\n\t\tbool __andNeeded_ = false;\n";
	loadNonPk << "\t\t__load_ << R\"(";

	persistence::Model::Fields fields;
	fields.reserve(table.fields.size());
	for (auto& e : table.fields) {
		fields.push_back(e.second);
	}
	loadNonPk << persistence::DBHandler::createSelect(fields, table.name);

	int nonPrimaryKeyMembers = 0;
	std::stringstream loadNonPkAdd;
	loadNonPk << " WHERE )\";\n";
	src << "\tbool select(";
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		if (f.isPrimaryKey()) {
			continue;
		}
		const std::string& cpptype = getCPPType(f.type, true, true);
		if (nonPrimaryKeyMembers > 0) {
			src << ", ";
		}
		src << cpptype << " " << f.name;
		if (nonPrimaryKeyMembers > 0) {
			src << " = nullptr";
		}
		++nonPrimaryKeyMembers;
		loadNonPk << "\t\tif (" << f.name << " != nullptr) {\n";
#if 0
		loadNonPk << "\t\t\t_m._" << f.name << " = ";
		if (f.type != persistence::Model::FieldType::PASSWORD && f.type != persistence::Model::FieldType::STRING) {
			loadNonPk << "*";
		}
		loadNonPk << f.name << ";\n";
#endif
		loadNonPk << "\t\t\tif (__andNeeded_) {\n";
		loadNonPk << "\t\t\t\t__load_ << \" AND \";\n";
		loadNonPk << "\t\t\t\t__andNeeded_ = false;\n";
		loadNonPk << "\t\t\t}\n";

		loadNonPk << "\t\t\t__load_ << \"" << quote << f.name << quote << " = ";
		loadNonPk << "$\" << __count_";
		loadNonPk << ";\n\t\t\t++__count_;\n\t\t\t__andNeeded_ = true;\n";
		loadNonPk << "\t\t}\n";

		loadNonPkAdd << "\t\tif (" << f.name << " != nullptr) {\n";
		if (f.type == persistence::Model::FieldType::TIMESTAMP) {
			loadNonPkAdd << "\t\t\tif (" << f.name << "->isNow()) {\n";
			loadNonPkAdd << "\t\t\t\t__p_.add(\"NOW()\");\n";
			loadNonPkAdd << "\t\t\t} else {\n";
			loadNonPkAdd << "\t\t\t\t__p_.add(*" << f.name << ");\n";
			loadNonPkAdd << "\t\t\t}\n";
		} else if (f.type == persistence::Model::FieldType::PASSWORD) {
			loadNonPkAdd << "\t\t\t__p_.addPassword(";
			loadNonPkAdd << f.name << ");\n";
		} else if (f.type == persistence::Model::FieldType::LONG || f.type == persistence::Model::FieldType::INT) {
			loadNonPkAdd << "\t\t\t__p_.add(*";
			loadNonPkAdd << f.name << ");\n";
		} else {
			loadNonPkAdd << "\t\t\t__p_.add(";
			loadNonPkAdd << f.name << ");\n";
		}
		loadNonPkAdd << "\t\t}\n";
	}
	if (nonPrimaryKeyMembers > 0) {
		src << ") {\n";
		src << loadNonPk.str();
		src << "\t\tconst std::string __load_str_ = __load_.str();\n";
		src << "\t\tSuper::PreparedStatement __p_ = prepare(\"\", __load_str_);\n";
		src << loadNonPkAdd.str();
		src << "\t\tconst State& __state = __p_.exec();\n";
		src << "\t\tcore_assert_msg(__state.result, \"Failed to execute statement: '%s' - error: '%s'\", __load_str_.c_str(), __state.lastErrorMsg.c_str());\n";
		src << "\t\treturn __state.result;\n\t}\n\n";
	}
}

static void createSelectByIds(const Table& table, std::stringstream& src) {
	persistence::Model::Fields fields;
	fields.reserve(table.fields.size());
	for (auto& e : table.fields) {
		fields.push_back(e.second);
	}
	const std::string& select = persistence::DBHandler::createSelect(fields, table.name);

	std::stringstream where;
	where << "WHERE ";

	std::stringstream loadadd;
	int fieldIndex = 0;
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		if (!f.isPrimaryKey()) {
			continue;
		}
		const std::string& cpptype = getCPPType(f.type, true);
		if (fieldIndex > 0) {
			src << ", ";
			where << " AND ";
		} else {
			src << "\tbool selectById(";
		}
		++fieldIndex;
		where << "\"" << f.name << "\" = ";
		sep(where, fieldIndex);
		loadadd << ".add(" << f.name << ")";
		src << cpptype << " " << f.name;
	}
	if (table.primaryKeys > 0) {
		src << ") {\n";
		src << "\t\tSuper::PreparedStatement __p_ = prepare(\"" << table.classname << "Load\",\n\t\t\tR\"("  << select << " " << where.str() << ")\");\n";
		src << "\t\t__p_" << loadadd.str() << ";\n";
		src << "\t\tconst State& __state = __p_.exec();\n";
		src << "\t\tcore_assert_msg(__state.result, \"Failed to execute selectById statement - error: '%s'\", __state.lastErrorMsg.c_str());\n";
		src << "\t\treturn __state.result;\n\t}\n\n";
	}
}

static void createInsertStatement(const Table& table, std::stringstream& src) {
	std::stringstream insert;
	std::stringstream insertvalues;
	std::stringstream insertadd;
	std::stringstream insertparams;
	std::string autoincrement;
	insert << "\"INSERT INTO " << quote << table.name << quote << " (";
	int insertValues = 0;
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		if (!f.isAutoincrement()) {
			if (insertValues > 0) {
				insertvalues << ", ";
				insert << ", ";
				insertparams << ", ";
			}
			++insertValues;
			insertparams << getCPPType(f.type, true) << " " << f.name;
			insert << "\\\"" << f.name << "\\\"";
			sep(insertvalues, insertValues);
			// TODO: length check if type is string
			if (f.type == persistence::Model::FieldType::PASSWORD) {
				insertadd << ".addPassword(" << f.name << ")";
			} else {
				insertadd << ".add(" << f.name << ")";
			}
		} else {
			autoincrement = f.name;
		}
	}

	insert << ") VALUES (" << insertvalues.str() << ")";
	if (!autoincrement.empty()) {
		insert << " RETURNING " << quote << autoincrement << quote;
	}
	// TODO: on duplicate key update
	insert << "\"";

	src << "\t/**\n";
	src << "\t * @brief Insert a new row into the database with the given parameters\n";
	src << "\t * @note Also fills the generated keys in the model instance\n";
	src << "\t * @return @c true if the execution was successful, @c false otherwise\n";
	src << "\t */\n";
	src << "\tbool insert(";
	src << insertparams.str();
	src << ") {\n";
	src << "\t\tSuper::PreparedStatement __p_ = prepare(\"" << table.classname << "Insert\",\n\t\t\t"  << insert.str() << ");\n";
	src << "\t\t__p_" << insertadd.str() << ";\n";
	src << "\t\treturn __p_.exec().result;\n";
	src << "\t}\n\n";
}

static void createGetterAndSetter(const Table& table, std::stringstream& src) {
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		const std::string& cpptypeGetter = getCPPType(f.type, true, isPointer(f));
		const std::string& cpptypeSetter = getCPPType(f.type, true, false);
		const std::string& n = core::string::upperCamelCase(f.name);

		src << "\tinline " << cpptypeGetter << " " << f.name << "() const {\n";
		if (isPointer(f)) {
			src << "\t\tif (_m._isNull_" << f.name << ") {\n";
			src << "\t\t\treturn nullptr;\n";
			src << "\t\t}\n";
			if (f.type == persistence::Model::FieldType::STRING || f.type == persistence::Model::FieldType::PASSWORD) {
				src << "\t\treturn _m._" << f.name << ".data();\n";
			} else {
				src << "\t\treturn &_m._" << f.name << ";\n";
			}
		} else {
			src << "\t\treturn _m._" << f.name << ";\n";
		}
		src << "\t}\n\n";

		src << "\tinline void set" << n << "(" << cpptypeSetter << " " << f.name << ") {\n";
		src << "\t\t_m._" << f.name << " = " << f.name << ";\n";
		if (isPointer(f)) {
			src << "\t\t_m._isNull_" << f.name << " = false;\n";
		}
		src << "\t}\n\n";

		if (isPointer(f)) {
			src << "\tinline void set" << n << "(nullptr_t " << f.name << ") {\n";
			src << "\t\t_m._isNull_" << f.name << " = true;\n";
			src << "\t}\n\n";
		}
	}
}

static void createCreateTableStatement(const Table& table, std::stringstream& src) {
	std::stringstream createTable;
	createTable << "CREATE TABLE IF NOT EXISTS " << quote << table.name << quote << " (\"\n";
	bool firstField = true;
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		if (!firstField) {
			createTable << ",\"\n";
		}
		createTable << "\t\t\t\"" << quote << f.name << quote;
		const std::string& dbType = persistence::DBHandler::getDbType(f);
		if (!dbType.empty()) {
			createTable << " " << dbType;
		}
		const std::string& flags = getDbFlags(table, f);
		if (!flags.empty()) {
			createTable << " " << flags;
		}
		firstField = false;
	}

	if (!table.uniqueKeys.empty()) {
		bool firstUniqueKey = true;
		for (const auto& uniqueKey : table.uniqueKeys) {
			createTable << ",\"\n\t\t\t\"UNIQUE(";
			for (const std::string& fieldName : uniqueKey) {
				if (!firstUniqueKey) {
					createTable << ", ";
				}
				createTable << quote << fieldName << quote;
				firstUniqueKey = false;
			}
			createTable << ")";
		}
	}

	if (table.primaryKeys > 1) {
		createTable << ",\"\n\t\t\t\"PRIMARY KEY(";
		bool firstPrimaryKey = true;
		for (auto entry : table.fields) {
			const persistence::Model::Field& f = entry.second;
			if (!f.isPrimaryKey()) {
				continue;
			}
			if (!firstPrimaryKey) {
				createTable << ", ";
			}
			createTable << quote << f.name << quote;
			firstPrimaryKey = false;
		}
		createTable << ")\"\n";
	} else {
		createTable << "\"\n";
	}
	createTable << "\t\t\t\");";

	src << "\tstatic bool createTable() {\n";
	src << "\t\treturn " << table.classname << "().exec(\"" << createTable.str() << "\");\n";
	src << "\t}\n";
}

bool generateClassForTable(const Table& table, std::stringstream& src) {
	src << "/**\n * @file\n */\n\n";
	src << "#pragma once\n\n";
	src << "#include \"persistence/Model.h\"\n";
	src << "#include \"core/String.h\"\n";
	src << "#include \"core/Common.h\"\n\n";
	src << "#include <memory>\n";
	src << "#include <vector>\n";
	src << "#include <array>\n";
	src << "#include <string>\n\n";

	const Namespace ns(table, src);
	const Class cl(table, src);

	src << "\tfriend class persistence::DBHandler;\n";
	src << "protected:\n";

	createMembersStruct(table, src);

	src << "public:\n";

	createMetaInformation(table, src);

	createConstructor(table, src);

	createSelectStatement(table, src);

	createSelectByIds(table, src);

	createInsertStatement(table, src);

	createCreateTableStatement(table, src);

	createGetterAndSetter(table, src);

	return true;
}

}
