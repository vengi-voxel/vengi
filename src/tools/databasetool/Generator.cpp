#include "Generator.h"
#include "Util.h"
#include "Mapping.h"
#include "Table.h"
#include "persistence/DBHandler.h"
#include "persistence/SQLGenerator.h"
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

	static std::string nullFieldName(const persistence::Field& f) {
		return "_isNull_" + f.name;
	}
};

static void createMembersStruct(const Table& table, std::stringstream& src) {
	src << "\tstruct " << MembersStruct::structName() << " {\n";
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
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

void createConstructor(const Table& table, std::stringstream& src) {
	src << "\t" << table.classname << "(";
	src << ") : Super(\"" << table.name << "\") {\n";
	src << "\t\t_membersPointer = (uint8_t*)&" << MembersStruct::varName() << ";\n";
	src << "\t\t_fields.reserve(" << table.fields.size() << ");\n";
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		src << "\t\t_fields.emplace_back(persistence::Field{";
		src << "\"" << f.name << "\"";
		src << ", persistence::FieldType::" << FieldTypeNames[std::enum_value(f.type)];
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
	src << "\t\t_constraints.reserve(" << table.constraints.size() << ");\n";
	for (auto i = table.constraints.begin(); i != table.constraints.end(); ++i) {
		const persistence::Constraint& c = i->second;
		src << "\t\t_constraints.insert(std::make_pair(\"" << i->first << "\", persistence::Constraint{{\"";
		src << core::string::join(c.fields.begin(), c.fields.end(), "\",\"");
		src << "\"}, " << c.types << "}));\n";
	}
	for (const auto& uniqueKey : table.uniqueKeys) {
		src << "\t\t_uniqueKeys.emplace_back(std::set<std::string>{\"";
		src << core::string::join(uniqueKey.begin(), uniqueKey.end(), "\", \"");
		src << "\"});\n";
	}

	src << "\t\t_primaryKeys = " << table.primaryKeys << ";\n";
	src << "\t}\n\n";

	src << "\t" << table.classname << "(" << table.classname << "&& source) : Super(std::move(source._tableName)) {\n";
	src << "\t\t_fields = std::move(source._fields);\n";
	src << "\t\t_primaryKeys = source._primaryKeys;\n";
	src << "\t\t_constraints = std::move(source._constraints);\n";
	src << "\t\t_uniqueKeys = std::move(source._uniqueKeys);\n";
	src << "\t\t_m = std::move(source._m);\n";
	src << "\t\t_membersPointer = (uint8_t*)&_m;\n";
	src << "\t}\n\n";

	src << "\t" << table.classname << "& operator=(" << table.classname << "&& source) {\n";
	src << "\t\t_tableName = std::move(source._tableName);\n";
	src << "\t\t_fields = std::move(source._fields);\n";
	src << "\t\t_primaryKeys = source._primaryKeys;\n";
	src << "\t\t_constraints = std::move(source._constraints);\n";
	src << "\t\t_uniqueKeys = std::move(source._uniqueKeys);\n";
	src << "\t\t_m = std::move(source._m);\n";
	src << "\t\t_membersPointer = (uint8_t*)&_m;\n";
	src << "\t\treturn *this;\n";
	src << "\t}\n\n";

	src << "\t" << table.classname << "(const " << table.classname << "& source) : Super(source._tableName) {\n";
	src << "\t\t_fields = source._fields;\n";
	src << "\t\t_primaryKeys = source._primaryKeys;\n";
	src << "\t\t_constraints = source._constraints;\n";
	src << "\t\t_uniqueKeys = source._uniqueKeys;\n";
	src << "\t\t_m = source._m;\n";
	src << "\t\t_membersPointer = (uint8_t*)&_m;\n";
	src << "\t}\n\n";
}

static void createSelectStatement(const Table& table, std::stringstream& src) {
	if (table.primaryKeys <= 0) {
		return;
	}
	std::stringstream loadNonPk;
	loadNonPk << "\t\tstd::stringstream __load_;\n\t\tint __count_ = 1;\n\t\tbool __andNeeded_ = false;\n";
	loadNonPk << "\t\t__load_ << ";

	persistence::Fields fields;
	fields.reserve(table.fields.size());
	for (auto& e : table.fields) {
		fields.push_back(e.second);
	}
	loadNonPk << "persistence::createSelect(*this) + ";

	int nonPrimaryKeyMembers = 0;
	std::stringstream loadNonPkAdd;
	loadNonPk << "\" WHERE \";\n";
	src << "\tbool select(";
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
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
		if (f.type != persistence::FieldType::PASSWORD && f.type != persistence::FieldType::STRING) {
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
		if (f.type == persistence::FieldType::TIMESTAMP) {
			loadNonPkAdd << "\t\t\tif (" << f.name << "->isNow()) {\n";
			loadNonPkAdd << "\t\t\t\t__p_.add(\"NOW()\");\n";
			loadNonPkAdd << "\t\t\t} else {\n";
			loadNonPkAdd << "\t\t\t\t__p_.add(*" << f.name << ");\n";
			loadNonPkAdd << "\t\t\t}\n";
		} else if (f.type == persistence::FieldType::PASSWORD) {
			loadNonPkAdd << "\t\t\t__p_.addPassword(";
			loadNonPkAdd << f.name << ");\n";
		} else if (f.type == persistence::FieldType::LONG || f.type == persistence::FieldType::INT) {
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
		src << "\t\tpersistence::PreparedStatement __p_ = prepare(\"\", __load_str_);\n";
		src << loadNonPkAdd.str();
		src << "\t\tconst persistence::State& __state = __p_.exec();\n";
		src << "\t\tcore_assert_msg(__state.result, \"Failed to execute statement: '%s' - error: '%s'\", __load_str_.c_str(), __state.lastErrorMsg);\n";
		src << "\t\treturn __state.result;\n\t}\n\n";
	}
}

static void createSelectByIds(const Table& table, std::stringstream& src) {
	persistence::Fields fields;
	fields.reserve(table.fields.size());
	for (auto& e : table.fields) {
		fields.push_back(e.second);
	}

	std::stringstream where;
	where << "WHERE ";

	std::stringstream loadadd;
	int fieldIndex = 0;
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
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
		src << "\t\tpersistence::PreparedStatement __p_ = prepare(\"" << table.classname << "Load\", persistence::createSelect(*this) + R\"( " << where.str() << ")\");\n";
		src << "\t\t__p_" << loadadd.str() << ";\n";
		src << "\t\tconst persistence::State& __state = __p_.exec();\n";
		src << "\t\tcore_assert_msg(__state.result, \"Failed to execute selectById statement - error: '%s'\", __state.lastErrorMsg);\n";
		src << "\t\treturn __state.result;\n\t}\n\n";
	}
}

static void createDBConditions(const Table& table, std::stringstream& src) {
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		const std::string classname = "DBCondition" + core::string::upperCamelCase(table.name) + core::string::upperCamelCase(f.name);
		src << "class " << classname;
		src << " : public persistence::DBCondition {\n";
		src << "private:\n";
		src << "\tusing Super = persistence::DBCondition;\n";
		src << "public:\n";
		src << "\t";
		if (f.type == persistence::FieldType::PASSWORD || f.type == persistence::FieldType::STRING || f.type == persistence::FieldType::TEXT) {
			src << "constexpr ";
		}
		src << classname << "(";
		if (f.type == persistence::FieldType::PASSWORD || f.type == persistence::FieldType::STRING || f.type == persistence::FieldType::TEXT) {
			src << "const char *";
		} else {
			src << getCPPType(f.type, true, false);
		}
		src << " value, persistence::Operator op = persistence::Operator::Equal) :\n\t\tSuper(\"";
		src << f.name << "\", ";
		if (f.type == persistence::FieldType::PASSWORD || f.type == persistence::FieldType::STRING || f.type == persistence::FieldType::TEXT) {
			src << "value";
		} else if (f.type == persistence::FieldType::TIMESTAMP) {
			src << "std::to_string(value.time())";
		} else {
			src << "std::to_string(value)";
		}
		src << ", op) {\n\t}\n";
		src << "}; // class " << classname << "\n\n";
	}
}

static void createGetterAndSetter(const Table& table, std::stringstream& src) {
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		const std::string& cpptypeGetter = getCPPType(f.type, true, isPointer(f));
		const std::string& cpptypeSetter = getCPPType(f.type, true, false);
		const std::string& n = core::string::upperCamelCase(f.name);

		src << "\tinline " << cpptypeGetter << " " << f.name << "() const {\n";
		if (isPointer(f)) {
			src << "\t\tif (_m._isNull_" << f.name << ") {\n";
			src << "\t\t\treturn nullptr;\n";
			src << "\t\t}\n";
			if (f.type == persistence::FieldType::STRING || f.type == persistence::FieldType::PASSWORD) {
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

bool generateClassForTable(const Table& table, std::stringstream& src) {
	src << "/**\n * @file\n */\n\n";
	src << "#pragma once\n\n";
	src << "#include \"persistence/Model.h\"\n";
	src << "#include \"persistence/SQLGenerator.h\"\n";
	src << "#include \"persistence/DBCondition.h\"\n";
	src << "#include \"core/String.h\"\n";
	src << "#include \"core/Common.h\"\n\n";
	src << "#include <memory>\n";
	src << "#include <vector>\n";
	src << "#include <array>\n";
	src << "#include <string>\n\n";

	const Namespace ns(table, src);
	{
		const Class cl(table, src);

		src << "\tfriend class persistence::DBHandler;\n";
		src << "protected:\n";

		createMembersStruct(table, src);

		src << "public:\n";

		createConstructor(table, src);

		createSelectStatement(table, src);

		createSelectByIds(table, src);

		createGetterAndSetter(table, src);
	}

	createDBConditions(table, src);

	return true;
}

}
