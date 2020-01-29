#include "Parser.h"
#include "Util.h"
#include "Mapping.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "core/Tokenizer.h"
#include "core/StringUtil.h"

namespace databasetool {

static const char *Keywords[] {
	"alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel",
	"atomic_commit", "atomic_noexcept", "auto", "bitand", "bitor", "bool",
	"break", "case", "catch", "char", "char16_t", "char32_t", "class",
	"compl", "concept", "const", "constexpr", "const_cast", "continue",
	"co_await", "co_return", "co_yield", "decltype", "default", "delete",
	"do", "double", "dynamic_cast", "else", "enum", "explicit", "export",
	"extern", "false", "float", "for", "friend", "goto", "if", "import",
	"inline", "int", "long", "module", "mutable", "namespace", "new",
	"noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq",
	"private", "protected", "public", "register", "reinterpret_cast",
	"requires", "return", "short", "signed", "sizeof", "static",
	"static_assert", "static_cast", "struct", "switch", "synchronized",
	"template", "this", "thread_local", "throw", "true", "try", "typedef",
	"typeid", "typename", "union", "unsigned", "using", "virtual", "void",
	"volatile", "wchar_t", "while", "xor", "xor_eq",
	nullptr
};

static bool checkFieldname(const std::string& in) {
	const char **token = Keywords;
	while (*token) {
		if (in == *token) {
			return false;
		}
		++token;
	}
	return true;
}

bool parseField(core::Tokenizer& tok, Table& table) {
	if (!tok.hasNext()) {
		Log::error("Expected field name");
		return false;
	}
	const std::string& fieldname = tok.next();
	if (!tok.hasNext()) {
		Log::error("Expected { after field name %s", fieldname.c_str());
		return false;
	}
	if (!checkFieldname(fieldname)) {
		Log::error("Field %s uses a reserved keyword", fieldname.c_str());
		return false;
	}
	std::string token = tok.next();
	if (token != "{") {
		Log::error("Expected {, found %s", token.c_str());
		return false;
	}
	persistence::Field field;
	field.name = fieldname;
	while (tok.hasNext()) {
		token = tok.next();
		if (token == "}") {
			break;
		}
		if (token == "type") {
			if (!tok.hasNext()) {
				Log::error("missing type for field %s", fieldname.c_str());
				return false;
			}
			const std::string& type = tok.next();
			const persistence::FieldType typeMapping = persistence::toFieldType(type);
			const bool foundType = typeMapping != persistence::FieldType::MAX;
			if (!foundType) {
				Log::error("invalid field type for field %s: %s", fieldname.c_str(), type.c_str());
				return false;
			}
			field.type = typeMapping;
		} else if (token == "default") {
			if (!tok.hasNext()) {
				Log::error("Missing value for default of %s", fieldname.c_str());
				return false;
			}
			if (!field.defaultVal.empty()) {
				Log::error("There is already a default value (%s) defined for field '%s'", field.defaultVal.c_str(), field.name.c_str());
				return false;
			}
			field.defaultVal = tok.next();
			if (core::string::iequals(field.defaultVal, "now()")) {
				field.defaultVal = "(NOW() AT TIME ZONE 'UTC')";
			}
		} else if (token == "operator") {
			if (!tok.hasNext()) {
				Log::error("missing operator for field %s", fieldname.c_str());
				return false;
			}
			const std::string& opStr = tok.next();
			persistence::Operator op = persistence::Operator::SET;
			bool foundOperator = false;
			for (int i = 0; i < (int)persistence::Operator::MAX; ++i) {
				if (core::string::iequals(opStr, OperatorNames[i])) {
					op = (persistence::Operator)i;
					foundOperator = true;
					break;
				}
			}
			if (!foundOperator) {
				Log::error("invalid operator for field %s: %s", fieldname.c_str(), opStr.c_str());
				return false;
			}
			field.updateOperator = op;
		} else if (token == "length") {
			if (!tok.hasNext()) {
				Log::error("Missing value for length of '%s'", fieldname.c_str());
				return false;
			}
			if (field.type != persistence::FieldType::STRING && field.type != persistence::FieldType::PASSWORD) {
				Log::error("Field '%s' of type '%s' doesn't support length parameter", fieldname.c_str(), persistence::toFieldType(field.type));
				return false;
			}
			// TODO: what is the min length for passwords? (see pgcrypto)
			field.length = core::string::toInt(tok.next());
		} else {
			uint32_t typeMapping = 0u;
			for (uint32_t i = 0; i < persistence::MAX_CONSTRAINTTYPES; ++i) {
				if (core::string::iequals(token, ConstraintTypeNames[i])) {
					typeMapping = 1 << i;
					break;
				}
			}
			if (typeMapping == 0u) {
				Log::error("Unknown token found in table definition: %s", token.c_str());
				return false;
			}

			auto i = table.constraints.find(field.name);
			if (i != table.constraints.end()) {
				persistence::Constraint& c = i->second;
				c.types |= typeMapping;
			} else {
				table.constraints.insert(std::make_pair(field.name, persistence::Constraint{{field.name}, typeMapping}));
			}
		}
	}
	table.fields.insert(std::make_pair(field.name, field));
	if (field.isLower()) {
		if (!isString(field)) {
			Log::error("'lowercase' specified for a none-string field: %s", field.name.c_str());
			return false;
		}
		if (field.type == persistence::FieldType::PASSWORD) {
			Log::error("'lowercase' specified for a password field: %s", field.name.c_str());
			return false;
		}
	}
	return true;
}

bool parseConstraints(core::Tokenizer& tok, Table& table) {
	if (!tok.hasNext()) {
		Log::error("Expected { after constraints");
		return false;
	}
	std::string token = tok.next();
	Log::trace("token: '%s'", token.c_str());
	if (token != "{") {
		Log::error("Expected {, found %s", token.c_str());
		return false;
	}
	while (tok.hasNext()) {
		std::set<std::string> fieldNames;
		token = tok.next();
		Log::trace("token: '%s'", token.c_str());
		if (token == "}") {
			break;
		}
		if (token == "(") {
			// parse token list
			while (tok.hasNext()) {
				token = tok.next();
				Log::trace("list token: '%s'", token.c_str());
				if (token == "," || token.empty()) {
					continue;
				}
				if (token == ")") {
					// this might happen because the separator and split char might follow each other
					if (!tok.hasNext()) {
						return false;
					}
					token = tok.next();
					if (!token.empty()) {
						token = tok.prev();
					}
					break;
				}
				fieldNames.insert(token);
			}
		} else {
			fieldNames.insert(token);
		}
		if (!tok.hasNext()) {
			Log::error("invalid constraint syntax for table %s", table.name.c_str());
			return false;
		}
		token = tok.next();
		Log::trace("type: '%s', table: %s", token.c_str(), table.name.c_str());
		uint32_t typeMapping = 0u;
		for (uint32_t i = 0; i < persistence::MAX_CONSTRAINTTYPES; ++i) {
			if (core::string::iequals(token, ConstraintTypeNames[i])) {
				typeMapping = 1 << i;
				break;
			}
		}
		if (typeMapping == 0u) {
			Log::error("invalid constraint syntax for table '%s': '%s' - there is no type mapping found", table.name.c_str(), token.c_str());
			return false;
		}

		if ((typeMapping & std::enum_value(persistence::ConstraintType::FOREIGNKEY)) != 0) {
			if (fieldNames.size() != 1) {
				Log::error("invalid foreign key constraint for table %s - expected to have exactly one field given",
						table.name.c_str());
				return false;
			}
			if (!tok.hasNext()) {
				Log::error("invalid foreign key constraint for table %s - expected foreign table", table.name.c_str());
				return false;
			}
			token = tok.next();
			if (!tok.hasNext()) {
				Log::error("invalid foreign key constraint for table %s - expected foreign field in table %s",
						table.name.c_str(), token.c_str());
				return false;
			}
			const persistence::ForeignKey fk{token, tok.next()};
			// there is only one entry
			const std::string& fieldName = *fieldNames.begin();
			table.foreignKeys.insert(std::make_pair(fieldName, fk));
		} else if ((typeMapping & std::enum_value(persistence::ConstraintType::AUTOINCREMENT)) != 0) {
			if (tok.hasNext()) {
				token = tok.next();
				const long startCounterLong = core::string::toLong(token);
				if (startCounterLong > 0) {
					core_assert_msg(table.autoIncrementStart == 1, "Table %s already has a auto increment starting value set", table.name.c_str());
					table.autoIncrementStart = startCounterLong;
				} else {
					tok.prev();
				}
			}
		}

		if (fieldNames.size() == 1) {
			const std::string& name = *fieldNames.begin();
			auto i = table.constraints.find(name);
			if (i != table.constraints.end()) {
				persistence::Constraint& c = i->second;
				c.types |= typeMapping;
			} else {
				Log::trace("fieldnames: %i", (int)fieldNames.size());
				std::vector<std::string> fieldNamesVec;
				std::copy(fieldNames.begin(), fieldNames.end(), std::back_inserter(fieldNamesVec));
				table.constraints.insert(std::make_pair(name, persistence::Constraint{fieldNamesVec, typeMapping}));
			}
		}
		if (typeMapping == (uint32_t)std::enum_value(persistence::ConstraintType::UNIQUE)) {
			table.uniqueKeys.emplace_back(std::move(fieldNames));
		}
	}
	return true;
}

bool parseTable(core::Tokenizer& tok, Table& table) {
	if (!tok.hasNext()) {
		Log::error("Expected {");
		return false;
	}
	std::string token = tok.next();
	if (token != "{") {
		Log::error("Expected {, found %s", token.c_str());
		return false;
	}
	while (tok.hasNext()) {
		token = tok.next();
		if (token == "}") {
			break;
		}
		if (token == "field") {
			if (!parseField(tok, table)) {
				return false;
			}
		} else if (token == "constraints") {
			if (!parseConstraints(tok, table)) {
				return false;
			}
		} else if (token == "namespace") {
			if (!tok.hasNext()) {
				Log::error("missing namespace name for table %s", table.name.c_str());
				return false;
			}
			table.namespaceSrc = tok.next();
		} else if (token == "schema") {
			if (!tok.hasNext()) {
				Log::error("missing schema name for table %s", table.name.c_str());
				return false;
			}
			table.schema = tok.next();
		} else if (token == "classname") {
			if (!tok.hasNext()) {
				Log::error("missing clasname name for table %s", table.name.c_str());
				return false;
			}
			table.classname = tok.next();
		} else {
			Log::error("Unknown token in table %s: %s", table.name.c_str(), token.c_str());
			return false;
		}
	}

	for (auto entry : table.constraints) {
		const persistence::Constraint& c = entry.second;
		for (const std::string& fieldName: c.fields) {
			if (table.fields.find(fieldName) == table.fields.end()) {
				Log::error("constraint referenced field wasn't found: '%s'", fieldName.c_str());
				return false;
			}
			Log::debug("transfer constraint to field for faster lookup for %s", fieldName.c_str());
			table.fields[fieldName].contraintMask |= c.types;
		}
		if ((c.types & std::enum_value(persistence::ConstraintType::PRIMARYKEY)) != 0) {
			table.primaryKeys += c.fields.size();
		}
	}

	for (auto field : table.fields) {
		if (field.second.isForeignKey() || field.second.isPrimaryKey()) {
			if (field.second.updateOperator != persistence::Operator::SET) {
				Log::error("invalid operator for primary or foreign key of field '%s' for table '%s'. Operator should be 'set'.",
						field.second.name.c_str(), table.name.c_str());
				return false;
			}
		}
	}

	sort(table.fields);

	return !table.fields.empty();
}

}
