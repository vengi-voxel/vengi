#include "Generator.h"
#include "Util.h"
#include "Mapping.h"
#include "core/String.h"

namespace databasetool {

static const char *quote = "\\\"";

struct Namespace {
	const databasetool::Table& _table;
	std::stringstream& _src;

	Namespace(const databasetool::Table& table, std::stringstream& src) : _table(table), _src(src) {
		if (!table.namespaceSrc.empty()) {
			src << "namespace " << table.namespaceSrc << " {\n\n";
		}
		src << "namespace persistence {\n\n";
	}
	~Namespace() {
		_src << "} // namespace ::persistence\n\n";
		if (!_table.namespaceSrc.empty()) {
			_src << "} // namespace " << _table.namespaceSrc << "\n\n";
		}
	}
};

struct Class {
	const databasetool::Table& _table;
	std::stringstream& _src;

	Class(const databasetool::Table& table, std::stringstream& src) : _table(table), _src(src) {
		src << "class " << table.classname << " : public ::persistence::Model {\n";
		src << "private:\n";
		src << "\tusing Super = ::persistence::Model;\n";
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

static void createMembersStruct(const databasetool::Table& table, std::stringstream& src) {
	src << "\tstruct " << MembersStruct::structName() << "{\n";
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		src << "\t\t";
		src << databasetool::getCPPType(f.type, false);
		src << " _" << f.name;
		if (databasetool::needsInitCPP(f.type)) {
			src << " = " << databasetool::getCPPInit(f.type, false);
		}
		src << ";\n";
		if (databasetool::isPointer(f)) {
			src << "\t\tbool " << MembersStruct::nullFieldName(f) << " = false;\n";
		}
	}
	src << "\t};\n";
	src << "\tMembers " << MembersStruct::varName() << ";\n";
}

void createConstructor(const databasetool::Table& table, std::stringstream& src) {
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
		if (databasetool::isPointer(f)) {
			src << ", offsetof(";
			src << MembersStruct::structName() << ", " << MembersStruct::nullFieldName(f) << ")";
		} else {
			src << ", -1";
		}
		src << "});\n";
	}
	src << "\t}\n\n";
}

static void createSelectStatement(const databasetool::Table& table, std::stringstream& src) {
	int nonPrimaryKeyMembers = 0;
	std::stringstream loadNonPk;
	std::stringstream loadNonPkAdd;

	loadNonPk << "\t\tstd::stringstream __load_;\n\t\tint __count_ = 1;\n\t\tbool __andNeeded_ = false;\n";

	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		if (f.isPrimaryKey()) {
			continue;
		}
		const std::string& cpptype = databasetool::getCPPType(f.type, true, true);
		if (nonPrimaryKeyMembers == 0) {
			src << "\tbool select(";
			loadNonPk << "\t\t__load_ << \"SELECT * FROM " << quote << table.name << quote << " WHERE \";\n";
		} else {
			src << ", ";
		}
		src << cpptype << " " << f.name;
		if (nonPrimaryKeyMembers != 0) {
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

		loadNonPk << "\t\t\t__load_ << \"" << f.name << " = ";
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

static void createSelectByIds(const databasetool::Table& table, std::stringstream& src) {
	std::stringstream select;
	std::stringstream loadadd;
	int fieldIndex = 0;
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		if (!f.isPrimaryKey()) {
			continue;
		}
		const std::string& cpptype = databasetool::getCPPType(f.type, true);
		if (fieldIndex > 0) {
			src << ", ";
			select << " AND ";
		} else {
			select << "\"SELECT * FROM " << quote << table.name << quote << " WHERE ";
			src << "\tbool selectById(";
		}
		++fieldIndex;
		select << quote << f.name << quote << " = ";
		databasetool::sep(select, fieldIndex);
		loadadd << ".add(" << f.name << ")";
		src << cpptype << " " << f.name;
	}
	if (table.primaryKeys > 0) {
		select << "\"";
		src << ") {\n";
		src << "\t\tSuper::PreparedStatement __p_ = prepare(\"" << table.classname << "Load\",\n\t\t\t"  << select.str() << ");\n";
		src << "\t\t__p_" << loadadd.str() << ";\n";
		src << "\t\tconst State& __state = __p_.exec();\n";
		src << "\t\tcore_assert_msg(__state.result, \"Failed to execute selectById statement - error: '%s'\", __state.lastErrorMsg.c_str());\n";
		src << "\t\treturn __state.result;\n\t}\n\n";
	}
}

static void createTruncateStatement(const databasetool::Table& table, std::stringstream& src) {
	src << "\tstatic bool truncate() {\n";
	src << "\t\treturn " << table.classname << "().exec(\"TRUNCATE TABLE " << quote << table.name << quote << "\");\n";
	src << "\t}\n\n";
}

static void createInsertStatement(const databasetool::Table& table, std::stringstream& src) {
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
			insertparams << databasetool::getCPPType(f.type, true) << " " << f.name;
			insert << "\\\"" << f.name << "\\\"";
			databasetool::sep(insertvalues, insertValues);
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
		insert << " RETURNING " << autoincrement;
	}
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

static void createGetterAndSetter(const databasetool::Table& table, std::stringstream& src) {
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		const std::string& cpptypeGetter = databasetool::getCPPType(f.type, true, databasetool::isPointer(f));
		const std::string& cpptypeSetter = databasetool::getCPPType(f.type, true, false);
		const std::string& n = core::string::upperCamelCase(f.name);

		src << "\tinline " << cpptypeGetter << " " << f.name << "() const {\n";
		if (databasetool::isPointer(f)) {
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
		if (databasetool::isPointer(f)) {
			src << "\t\t_m._isNull_" << f.name << " = false;\n";
		}
		src << "\t}\n\n";

		if (databasetool::isPointer(f)) {
			src << "\tinline void set" << n << "(nullptr_t " << f.name << ") {\n";
			src << "\t\t_m._isNull_" << f.name << " = true;\n";
			src << "\t}\n\n";
		}
	}
}

static void createCreateTableStatement(const databasetool::Table& table, std::stringstream& src) {
	std::stringstream createTable;
	createTable << "CREATE TABLE IF NOT EXISTS " << quote << table.name << quote << " (\"\n";
	bool firstField = true;
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		if (!firstField) {
			createTable << ",\"\n";
		}
		createTable << "\t\t\t\"" << f.name << " " << databasetool::getDbType(f) << getDbFlags(table, f);
		firstField = false;
	}

	if (!table.uniqueKeys.empty()) {
		bool firstUniqueKey = true;
		createTable << ",\"\n\t\t\t\"UNIQUE(";
		for (const std::string& fieldName : table.uniqueKeys) {
			if (!firstUniqueKey) {
				createTable << ", ";
			}
			createTable << fieldName;
			firstUniqueKey = false;
		}
		createTable << ")";
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
			createTable << f.name;
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

bool generateClassForTable(const databasetool::Table& table, std::stringstream& src) {
	const Namespace ns(table, src);
	const Class cl(table, src);

	src << "protected:\n";

	createMembersStruct(table, src);

	src << "public:\n";

	createConstructor(table, src);

	createSelectStatement(table, src);

	createSelectByIds(table, src);

	createInsertStatement(table, src);

	createTruncateStatement(table, src);

	createCreateTableStatement(table, src);

	createGetterAndSetter(table, src);

	return true;
}

}
