/**
 * @file
 */

#include "Generator.h"
#include "Util.h"
#include "Mapping.h"
#include "Table.h"
#include "core/StringUtil.h"
#include "core/Assert.h"

namespace databasetool {

#define NAMESPACE "db"

struct Namespace {
	const Table& _table;
	core::String& _src;

	Namespace(const Table& table, core::String& src) : _table(table), _src(src) {
		if (!table.namespaceSrc.empty()) {
			src += "namespace " + table.namespaceSrc + " {\n\n";
		}
		src += "namespace " NAMESPACE " {\n\n";
	}
	~Namespace() {
		_src += "typedef std::shared_ptr<" + _table.classname + "> " + _table.classname + "Ptr;\n\n";
		_src += "} // namespace " NAMESPACE "\n\n";
		if (!_table.namespaceSrc.empty()) {
			_src += "} // namespace " + _table.namespaceSrc + "\n\n";
		}
	}
};

struct Class {
	const Table& _table;
	core::String& _src;

	Class(const Table& table, core::String& src) : _table(table), _src(src) {
		src += "/**\n";
		src += " * @brief Model class for table '" + table.schema + "." + table.name + "'\n";
		src += " * @note Work with this class in combination with the persistence::DBHandler\n";
		src += " * @ingroup Persistence\n";
		src += " */\n";
		src += "class " + table.classname + " : public persistence::Model {\n";
		src += "private:\n";
		src += "\tusing Super = persistence::Model;\n";
	}

	~Class() {
		_src += "}; // class " + _table.classname + "\n\n";
	}
};

struct MembersStruct {
	static const char *structName() {
		return "Members";
	}

	static const char *varName() {
		return "_m";
	}

	static core::String nullFieldName(const persistence::Field& f) {
		return "_isNull_" + f.name;
	}

	static core::String validFieldName(const persistence::Field& f) {
		return "_isValid_" + f.name;
	}
};

static core::String getFieldNameFunction(const persistence::Field& field) {
	return "f_" + field.name;
}

static void createMembersStruct(const Table& table, core::String& src) {
	src += "\tstruct ";
	src += MembersStruct::structName();
	src += " {\n";
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		src += "\t\t/**\n";
		src += "\t\t * @brief Member for table column '" + entry.second.name + "'\n";
		src += "\t\t */\n";
		src += "\t\t";
		src += getCPPType(f.type, false);
		src += " _" + f.name;
		if (needsInitCPP(f.type)) {
			src += " = " + getCPPInit(f.type, false);
		}
		src += ";\n";
		// TODO: padding for short and boolean
	}
	for (auto entry : table.fields) {
		// TODO: use bitfield
		const persistence::Field& f = entry.second;
		if (isPointer(f)) {
			src += "\t\t/**\n";
			src += "\t\t * @brief Is the value set to null?\n";
			src += "\t\t * @c true if a value is set to null and the field should be taken into account for e.g. update statements, @c false if not\n";
			src += "\t\t */\n";
			src += "\t\tbool " + MembersStruct::nullFieldName(f) + " = false;\n";
		}
		src += "\t\t/**\n";
		src += "\t\t * @brief Is there a valid value set?\n";
		src += "\t\t * @c true if a value is set and the field should be taken into account for e.g. update statements, @c false if not\n";
		src += "\t\t */\n";
		src += "\t\tbool " + MembersStruct::validFieldName(f) + " = false;\n";
	}
	src += "\t};\n";
	src += "\tMembers ";
	src += MembersStruct::varName();
	src += ";\n";
}

static void createMetaStruct(const Table& table, core::String& src) {
	src += "\tstruct MetaPriv : public Meta {\n";
	src += "\t\tMetaPriv() {\n";

	src += "\t\t\t_schema = \"" + table.schema + "\";\n";
	src += "\t\t\t_tableName = \"" + table.name + "\";\n";
	src += "\t\t\t_primaryKeyFields = ";
	src += std::to_string(table.primaryKeys);
	src += ";\n";
	src += "\t\t\t_autoIncrementStart = ";
	src += std::to_string(table.autoIncrementStart);
	src += ";\n";
	src += "\t\t\t_fields.reserve(";
	src += std::to_string(table.fields.size());
	src += ");\n";
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		src += "\t\t\t_fields.emplace_back(persistence::Field{";
		src += "\"" + f.name + "\"";
		src += ", persistence::FieldType::";
		src += persistence::toFieldType(f.type);
		src += ", persistence::Operator::";
		src += OperatorNames[std::enum_value(f.updateOperator)];
		src += ", " + std::to_string(f.contraintMask);
		src += ", \"" + f.defaultVal + "\"";
		src += ", " + std::to_string(f.length);
		src += ", offsetof(";
		src += MembersStruct::structName();
		src += ", _";
		src += f.name;
		src += ")";
		if (isPointer(f)) {
			src += ", offsetof(";
			src += MembersStruct::structName();
			src += ", ";
			src += MembersStruct::nullFieldName(f);
			src += ")";
		} else {
			src += ", -1";
		}
		src += ", offsetof(";
		src += MembersStruct::structName();
		src += ", ";
		src += MembersStruct::validFieldName(f);
		src += ")";
		src += "});\n";
	}
	if (!table.constraints.empty()) {
		src += "\t\t\t_constraints.reserve(";
		src +=std::to_string(table.constraints.size());
		src += ");\n";
	}
	for (auto i = table.constraints.begin(); i != table.constraints.end(); ++i) {
		const persistence::Constraint& c = i->second;
		src += "\t\t\t_constraints.insert(std::make_pair(\"";
		src += i->first;
		src += "\", persistence::Constraint{{\"";
		src += core::string::join(c.fields.begin(), c.fields.end(), "\",\"");
		src += "\"}, ";
		src += std::to_string(c.types);
		src += "}));\n";
	}
	if (table.primaryKeys > 0) {
		src += "\t\t\t_primaryKeys.reserve(";
		src += std::to_string(table.primaryKeys);
		src += ");\n";
		for (auto entry : table.constraints) {
			const persistence::Constraint& c = entry.second;
			if ((c.types & std::enum_value(persistence::ConstraintType::PRIMARYKEY)) == 0) {
				continue;
			}
			for (const core::String& pkfield : c.fields) {
				src += "\t\t\t_primaryKeys.emplace_back(\"";
				src += pkfield;
				src += "\");\n";
			}
		}
	}
	for (auto entry : table.constraints) {
		const persistence::Constraint& c = entry.second;
		if ((c.types & std::enum_value(persistence::ConstraintType::AUTOINCREMENT)) == 0) {
			continue;
		}
		src += "\t\t\t_autoIncrementField = \"";
		src +=c.fields.front();
		src += "\";\n";
	}
	if (!table.uniqueKeys.empty()) {
		src += "\t\t\t_uniqueKeys.reserve(";
		src += std::to_string(table.uniqueKeys.size());
		src += ");\n";
	}
	for (const auto& uniqueKey : table.uniqueKeys) {
		src += "\t\t\t_uniqueKeys.emplace_back(std::set<core::String>{\"";
		src += core::string::join(uniqueKey.begin(), uniqueKey.end(), "\", \"");
		src += "\"});\n";
	}
	if (!table.foreignKeys.empty()) {
		src += "\t\t\t_foreignKeys.reserve(";
		src += std::to_string(table.foreignKeys.size());
		src += ");\n";
	}
	for (const auto& foreignKeyEntry : table.foreignKeys) {
		src += "\t\t\t_foreignKeys.insert(std::make_pair(\"";
		src += foreignKeyEntry.first;
		src += "\", persistence::ForeignKey{\"";
		src += foreignKeyEntry.second.table;
		src += "\", \"";
		src += foreignKeyEntry.second.field;
		src += "\"}));\n";
	}

	src += "\t\t}\n";
	src += "\t};\n";
	src += "\tstatic inline const Meta* meta() {\n\t\tstatic MetaPriv _meta;\n\t\treturn &_meta;\n\t}\n";
}

void createConstructor(const Table& table, core::String& src) {
	src += "\t" + table.classname + "(";
	src += ") : Super(meta()) {\n";
	src += "\t\t_membersPointer = (uint8_t*)&";
	src += MembersStruct::varName();
	src += ";\n";
	src += "\t}\n\n\t";

	src += table.classname;
	src += "(";
	src += table.classname;
	src += "&& source) : Super(meta()) {\n";
	src += "\t\t_m = std::move(source._m);\n";
	src += "\t\t_membersPointer = (uint8_t*)&_m;\n";
	src += "\t}\n\n\t";
	src += table.classname;
	src += "(const ";
	src += table.classname;
	src += "& source) : Super(meta()) {\n";
	src += "\t\t_m = source._m;\n";
	src += "\t\t_membersPointer = (uint8_t*)&_m;\n";
	src += "\t}\n\n\t";

	src += table.classname;
	src += "& operator=(";
	src += table.classname;
	src += "&& source) {\n";
	src += "\t\t_m = std::move(source._m);\n";
	src += "\t\t_membersPointer = (uint8_t*)&_m;\n";
	src += "\t\treturn *this;\n";
	src += "\t}\n\n\t";

	src += table.classname;
	src += "& operator=(const ";
	src += table.classname;
	src += "& source) {\n";
	src += "\t\t_m = source._m;\n";
	src += "\t\t_membersPointer = (uint8_t*)&_m;\n";
	src += "\t\treturn *this;\n";
	src += "\t}\n\n";
}

static void createDBConditions(const Table& table, core::String& src) {
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		if (f.type == persistence::FieldType::BLOB) {
			continue;
		}
		const core::String classname = "DBCondition" + core::string::upperCamelCase(table.classname) + core::string::upperCamelCase(f.name);
		src += "/**\n";
		src += " * @brief Condition for '" + table.schema + "." + table.name + "." + f.name + "'.\n";
		src += " */\n";
		src += "class " + classname;
		src += " : public persistence::DBCondition {\n";
		src += "private:\n";
		src += "\tusing Super = persistence::DBCondition;\n";
		src += "public:\n";
		src += "\t/**\n\t * @brief Condition for " + f.name + "\n";
		src += "\t * @param[in] value";
		if (f.type == persistence::FieldType::TIMESTAMP) {
			src += " UTC timestamp in seconds";
		} else if (isString(f)) {
			if (f.isLower()) {
				src += " The given value is converted to lowercase before the comparison takes place";
			}
		}
		src += "\n";
		src += "\t * @param[in] comp @c persistence::Comparator";
		src += "\n\t */\n\t";
		if (isString(f) && !f.isLower()) {
			src += "constexpr ";
		}
		src += classname + "(";
		if (isString(f)) {
			src += "const char *";
		} else {
			src += getCPPType(f.type, true, false);
		}
		src += " value, persistence::Comparator comp = persistence::Comparator::Equal) :\n\t\tSuper(";
		src += table.classname + "::" + getFieldNameFunction(f) + "(), persistence::FieldType::";
		src += persistence::toFieldType(f.type);
		src += ", ";
		if (isString(f)) {
			if (f.isLower()) {
				src += "persistence::toLower(value)";
			} else {
				src += "value";
			}
		} else if (f.type == persistence::FieldType::TIMESTAMP) {
			src += "std::to_string(value.seconds())";
		} else {
			src += "std::to_string(value)";
		}
		src += ", comp) {\n\t}\n";

		if (isString(f)) {
			src += "\t" + classname + "(";
			src += "const core::String&";
			src += " value, persistence::Comparator comp = persistence::Comparator::Equal) :\n\t\tSuper(";
			src += table.classname + "::f_" + f.name + "(), persistence::FieldType::";
			src += persistence::toFieldType(f.type);
			src += ", ";
			if (f.isLower()) {
				src += "persistence::toLower(value)";
			} else {
				src += "value";
			}
			src += ", comp) {\n\t}\n";
		}

		src += "}; // class " + classname + "\n\n";
	}
}

static void createDoxygen(const Table& table, const persistence::Field& f, core::String& src) {
	if (f.type == persistence::FieldType::TIMESTAMP) {
		src += "\t * @note The value is in seconds\n";
	}
	if (f.isAutoincrement()) {
		src += "\t * @note Auto increment\n";
	}
	if (f.isIndex()) {
		src += "\t * @note Index\n";
	}
	if (f.isNotNull()) {
		src += "\t * @note May not be null\n";
	}
	if (f.isPrimaryKey()) {
		src += "\t * @note Primary key\n";
	}
	if (f.isLower()) {
		src += "\t * @note Store as lowercase string\n";
	}
	if (f.isUnique()) {
		src += "\t * @note Unique key\n";
	}
	if (f.isForeignKey()) {
		auto i = table.foreignKeys.find(f.name);
		core_assert(i != table.foreignKeys.end());
		src += "\t * @note Foreign key to '" + table.schema + "." + i->second.table + "." + i->second.field + "'\n";
	}
	if (f.updateOperator == persistence::Operator::ADD) {
		src += "\t * @note Will add to the value in the conflict case (Operator::ADD)\n";
	} else if (f.updateOperator == persistence::Operator::SUBTRACT) {
		src += "\t * @note Will subtract to the value in the conflict case (Operator::SUBTRACT)\n";
	} else if (f.updateOperator == persistence::Operator::SET) {
		src += "\t * @note Will set the value in the conflict case (Operator::SET)\n";
	}
}

static void createGetterAndSetter(const Table& table, core::String& src) {
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		const core::String& cpptypeGetter = getCPPType(f.type, true, isPointer(f));
		const core::String& getter = core::string::lowerCamelCase(f.name);
		const core::String& cpptypeSetter = getCPPType(f.type, true, false);
		const core::String& setter = core::string::upperCamelCase(f.name);

		src += "\t/**\n\t * @brief Access the value for ";
		src += "'" + table.schema + "." + table.name + "." + f.name + "'";
		src += " after the model was loaded\n";
		createDoxygen(table, f, src);
		src += "\t */\n";

		src += "\tinline " + cpptypeGetter + " " + getter + "() const {\n";
		if (isPointer(f)) {
			src += "\t\tif (_m._isNull_" + f.name + ") {\n";
			src += "\t\t\treturn nullptr;\n";
			src += "\t\t}\n";
			if (isString(f)) {
				src += "\t\treturn _m._" + f.name + ".data();\n";
			} else {
				src += "\t\treturn &_m._" + f.name + ";\n";
			}
		} else {
			src += "\t\treturn _m._" + f.name + ";\n";
		}
		src += "\t}\n\n";

		src += "\t/**\n";
		src += "\t * @brief Set the value for ";
		src += "'" + table.schema + "." + table.name + "." + f.name + "'";
		src += " for updates, inserts and where clauses\n";
		createDoxygen(table, f, src);
		src += "\t */\n";
		src += "\tinline void set" + setter + "(" + cpptypeSetter + " " + f.name + ") {\n";
		src += "\t\t_m._" + f.name + " = ";
		if (isString(f) && f.isLower()) {
			src += "persistence::toLower(" + f.name + ")";
		} else {
			src += f.name;
		}
		src += ";\n";
		src += "\t\t_m." + MembersStruct::validFieldName(f) + " = true;\n";
		if (isPointer(f)) {
			src += "\t\t_m." + MembersStruct::nullFieldName(f) + " = false;\n";
		}
		src += "\t}\n\n";

		if (f.type == persistence::FieldType::INT || f.type == persistence::FieldType::SHORT) {
			src += "\t/**\n\t * @brief Set the value for '" + f.name + "' for updates and where clauses\n\t */\n";
			src += "\ttemplate<typename T, class = typename std::enable_if<std::is_enum<T>::value>::type>\n";
			src += "\tinline void set" + setter + "(const T& " + f.name + ") {\n";
			src += "\t\tset" + setter + "(static_cast<" + cpptypeSetter + ">(static_cast<typename std::underlying_type<T>::type>(" + f.name + ")));\n";
			src += "\t}\n\n";
		}

		if (isPointer(f)) {
			src += "\t/**\n\t * @brief Set the value for '" + f.name + "' for updates and where clauses to null\n\t */\n";
			src += "\tinline void set" + setter + "(std::nullptr_t " + f.name + ") {\n";
			src += "\t\t_m." + MembersStruct::nullFieldName(f) + " = true;\n";
			src += "\t\t_m." + MembersStruct::validFieldName(f) + " = true;\n";
			src += "\t}\n\n";
		}
	}
}

void createFieldNames(const Table& table, core::String& src) {
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		src += "\t/**\n";
		src += "\t * @brief The column name for '" + f.name + "'\n";
		src += "\t */\n";
		src += "\tstatic constexpr const char* " + getFieldNameFunction(f) + "() {\n\t\treturn \"" + f.name + "\";\n\t}\n\n";
	}
}

bool generateClassForTable(const Table& table, core::String& src) {
	src += "/**\n * @file\n */\n\n";
	src += "#pragma once\n\n";
	src += "#include \"persistence/Model.h\"\n";
	src += "#include \"persistence/DBCondition.h\"\n";
	src += "\n";
	src += "#include <memory>\n";

	const Namespace ns(table, src);
	{
		const Class cl(table, src);

		src += "\tfriend class persistence::DBHandler;\n";
		src += "protected:\n";

		createMembersStruct(table, src);

		createMetaStruct(table, src);

		src += "public:\n";

		createConstructor(table, src);

		createGetterAndSetter(table, src);

		createFieldNames(table, src);
	}

	createDBConditions(table, src);

	return true;
}

}
