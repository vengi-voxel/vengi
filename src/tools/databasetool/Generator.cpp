#include "Generator.h"
#include "Util.h"
#include "Mapping.h"
#include "Table.h"
#include "persistence/DBHandler.h"
#include "persistence/SQLGenerator.h"
#include "core/String.h"

namespace databasetool {

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

static std::string getFieldnameFunction(const persistence::Field& field) {
	return "f_" + field.name;
}

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
		// TODO: padding for short and boolean
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
		src << ", persistence::Operator::" << OperatorNames[std::enum_value(f.updateOperator)];
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
	if (!table.constraints.empty()) {
		src << "\t\t_constraints.reserve(" << table.constraints.size() << ");\n";
	}
	for (auto i = table.constraints.begin(); i != table.constraints.end(); ++i) {
		const persistence::Constraint& c = i->second;
		src << "\t\t_constraints.insert(std::make_pair(\"" << i->first << "\", persistence::Constraint{{\"";
		src << core::string::join(c.fields.begin(), c.fields.end(), "\",\"");
		src << "\"}, " << c.types << "}));\n";
	}
	if (!table.uniqueKeys.empty()) {
		src << "\t\t_uniqueKeys.reserve(" << table.uniqueKeys.size() << ");\n";
	}
	for (const auto& uniqueKey : table.uniqueKeys) {
		src << "\t\t_uniqueKeys.emplace_back(std::set<std::string>{\"";
		src << core::string::join(uniqueKey.begin(), uniqueKey.end(), "\", \"");
		src << "\"});\n";
	}
	if (!table.foreignKeys.empty()) {
		src << "\t\t_foreignKeys.reserve(" << table.foreignKeys.size() << ");\n";
	}
	for (const auto& foreignKeyEntry : table.foreignKeys) {
		src << "\t\t_foreignKeys.insert(std::make_pair(\"" << foreignKeyEntry.first << "\", persistence::ForeignKey{\"";
		src << foreignKeyEntry.second.table << "\", \"" << foreignKeyEntry.second.field;
		src << "\"}));\n";
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
		src << " value, persistence::Comparator comp = persistence::Comparator::Equal) :\n\t\tSuper(";
		src << table.classname << "::" << getFieldnameFunction(f) << "(), ";
		if (f.type == persistence::FieldType::PASSWORD || f.type == persistence::FieldType::STRING || f.type == persistence::FieldType::TEXT) {
			src << "value";
		} else if (f.type == persistence::FieldType::TIMESTAMP) {
			src << "std::to_string(value.seconds())";
		} else {
			src << "std::to_string(value)";
		}
		src << ", comp) {\n\t}\n";

		if (f.type == persistence::FieldType::PASSWORD || f.type == persistence::FieldType::STRING || f.type == persistence::FieldType::TEXT) {
			src << "\t" << classname << "(";
			src << "const std::string&";
			src << " value, persistence::Comparator comp = persistence::Comparator::Equal) :\n\t\tSuper(";
			src << table.classname << "::f_" << f.name << "(), value, comp) {\n\t}\n";
		}

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

		if (f.type == persistence::FieldType::INT || f.type == persistence::FieldType::SHORT) {
			src << "\ttemplate<typename T, class = typename std::enable_if<std::is_enum<T>::value>::type>\n";
			src << "\tinline void set" << n << "(const T& " << f.name << ") {\n";
			src << "\t\tset" << n << "(static_cast<" << cpptypeSetter << ">(static_cast<typename std::underlying_type<T>::type>(" << f.name << ")));\n";
			src << "\t}\n\n";
		}

		if (isPointer(f)) {
			src << "\tinline void set" << n << "(nullptr_t " << f.name << ") {\n";
			src << "\t\t_m._isNull_" << f.name << " = true;\n";
			src << "\t}\n\n";
		}
	}
}

void createFieldNames(const Table& table, std::stringstream& src) {
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		src << "\tstatic constexpr const char* " << getFieldnameFunction(f) << "() {\n\t\treturn \"" << f.name << "\";\n\t}\n\n";
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

		createGetterAndSetter(table, src);

		createFieldNames(table, src);
	}

	createDBConditions(table, src);

	return true;
}

}
