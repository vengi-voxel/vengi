/**
 * @file
 */

#include "DatabaseTool.h"
#include "io/Filesystem.h"
#include "core/String.h"

static const char *FieldTypeNames[] = {
	CORE_STRINGIFY(STRING),
	CORE_STRINGIFY(LONG),
	CORE_STRINGIFY(INT),
	CORE_STRINGIFY(PASSWORD),
	CORE_STRINGIFY(TIMESTAMP)
};
static_assert(SDL_arraysize(FieldTypeNames) == persistence::Model::MAX_FIELDTYPES, "Invalid field type mapping");

static const char *ConstraintTypeNames[] = {
	CORE_STRINGIFY(UNIQUE),
	CORE_STRINGIFY(PRIMARYKEY),
	CORE_STRINGIFY(AUTOINCREMENT),
	CORE_STRINGIFY(NOTNULL)
};
static_assert(SDL_arraysize(ConstraintTypeNames) == persistence::Model::MAX_CONSTRAINTTYPES, "Invalid constraint type mapping");

// TODO:
// * add update and delete methods
// * split into util classes
// * move sql syntax related methods into vendor specific classes
DatabaseTool::DatabaseTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider, 0) {
	init(ORGANISATION, "databasetool");
}

bool DatabaseTool::needsInitCPP(persistence::Model::FieldType type) const {
	switch (type) {
	case persistence::Model::FieldType::PASSWORD:
	case persistence::Model::FieldType::STRING:
	case persistence::Model::FieldType::TIMESTAMP:
		return false;
	default:
		return true;
	}
}

std::string DatabaseTool::getCPPInit(persistence::Model::FieldType type, bool pointer) const {
	if (pointer) {
		return "nullptr";
	}
	switch (type) {
	case persistence::Model::FieldType::PASSWORD:
	case persistence::Model::FieldType::STRING:
		return "\"\"";
	case persistence::Model::FieldType::TIMESTAMP:
	case persistence::Model::FieldType::LONG:
		return "0l";
	case persistence::Model::FieldType::INT:
		return "0";
	case persistence::Model::FieldType::MAX:
		break;
	}
	return "";
}

std::string DatabaseTool::getCPPType(persistence::Model::FieldType type, bool function, bool pointer) const {
	switch (type) {
	case persistence::Model::FieldType::PASSWORD:
	case persistence::Model::FieldType::STRING:
		if (pointer) {
			return "const char*";
		}
		if (function) {
			return "const std::string&";
		}
		return "std::string";
	case persistence::Model::FieldType::TIMESTAMP:
		if (function) {
			if (pointer) {
				return "const ::persistence::Timestamp*";
			}
			return "const ::persistence::Timestamp&";
		}
		return "::persistence::Timestamp";
	case persistence::Model::FieldType::LONG:
		if (pointer) {
			return "int64_t*";
		}
		return "int64_t";
	case persistence::Model::FieldType::INT:
		if (pointer) {
			return "int32_t*";
		}
		return "int32_t";
	case persistence::Model::FieldType::MAX:
		break;
	}
	return "";
}

std::string DatabaseTool::getDbType(const persistence::Model::Field& field) const {
	switch (field.type) {
	case persistence::Model::FieldType::PASSWORD:
	case persistence::Model::FieldType::STRING: {
		std::string type = "CHAR(";
		if (field.length > 0) {
			type += std::to_string(field.length);
		} else {
			type += "256";
		}
		type += ')';
		return type;
	}
	case persistence::Model::FieldType::TIMESTAMP:
		return "TIMESTAMP";
	case persistence::Model::FieldType::LONG:
		if (field.isAutoincrement()) {
			return "";
		}
		return "BIGINT";
	case persistence::Model::FieldType::INT:
		if (field.isAutoincrement()) {
			return "";
		}
		return "INT";
	case persistence::Model::FieldType::MAX:
		break;
	}
	return "";
}

std::string DatabaseTool::getDbFlags(const Table& table, const persistence::Model::Field& field) const {
	std::stringstream ss;
	if (field.isAutoincrement()) {
		if (field.type == persistence::Model::FieldType::LONG) {
			ss << " BIGSERIAL";
		} else {
			ss << " SERIAL";
		}
	}
	if (field.isNotNull()) {
		ss << " NOT NULL";
	}
	if (field.isPrimaryKey() && table.primaryKeys == 1) {
		ss << " PRIMARY KEY";
	}
	if (field.isUnique()) {
		ss << " UNIQUE";
	}
	if (!field.defaultVal.empty()) {
		ss << " DEFAULT " << field.defaultVal;
	}
	return ss.str();
}

void DatabaseTool::sep(std::stringstream& ss, int count) const {
	ss << "$" << count;
}

bool DatabaseTool::isPointer(const persistence::Model::Field& field) const {
	if (field.isNotNull()) {
		return false;
	}
	if (field.isPrimaryKey()) {
		return false;
	}
	if (field.isAutoincrement()) {
		return false;
	}
	if (field.isUnique()) {
		return false;
	}
	return true;
}

bool DatabaseTool::generateClassForTable(const Table& table, std::stringstream& src) const {
	if (!table.namespaceSrc.empty()) {
		src << "namespace " << table.namespaceSrc << " {\n\n";
	}
	src << "namespace persistence {\n\n";
	const std::string classname = table.classname;
	src << "class " << classname << " : public ::persistence::Model {\n";
	src << "private:\n";
	src << "\tusing Super = ::persistence::Model;\n";
	src << "protected:\n";

	src << "\tstruct Members {\n";
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
			src << "\t\tbool _isNull_" << f.name << " = false;\n;";
		}
	}
	src << "\t};\n";
	src << "\tMembers _m;\n";

	src << "public:\n";

	// ctor
	src << "\t" << classname << "(";
	src << ") : Super(\"" << table.name << "\") {\n";
	src << "\t\t_membersPointer = (uint8_t*)&_m;\n";
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		src << "\t\t_fields.push_back(Field{";
		src << "\"" << f.name << "\"";
		src << ", FieldType::" << FieldTypeNames[std::enum_value(f.type)];
		src << ", " << f.contraintMask;
		src << ", \"" << f.defaultVal << "\"";
		src << ", " << f.length;
		src << ", offsetof(";
		src << "Members, _" << f.name << ")";
		if (isPointer(f)) {
			src << ", offsetof(";
			src << "Members, _isNull_" << f.name << ")";
		} else {
			src << ", 0";
		}
		src << "});\n";
	}
	src << "\t}\n\n";

	// ctor for non primary keys
	int nonPrimaryKeyMembers = 0;
	std::stringstream loadNonPk;
	std::stringstream loadNonPkAdd;

	loadNonPk << "\t\tstd::stringstream __load_;\n\t\tint __count_ = 1;\n\t\tbool __andNeeded_ = false;\n";

	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		if (f.isPrimaryKey()) {
			continue;
		}
		const std::string& cpptype = getCPPType(f.type, true, true);
		if (nonPrimaryKeyMembers == 0) {
			src << "\tbool select(";
			loadNonPk << "\t\t__load_ << \"SELECT * FROM " << table.name << " WHERE \";\n";
		} else {
			src << ", ";
		}
		src << cpptype << " " << f.name;
		if (nonPrimaryKeyMembers != 0) {
			src << " = nullptr";
		}
		++nonPrimaryKeyMembers;
		loadNonPk << "\t\tif (" << f.name << " != nullptr) {\n";
		loadNonPk << "\t\t\t_m._" << f.name << " = *" << f.name << ";\n";
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

	// ctor for primary keys
	int primaryKeys = 0;
	std::stringstream insert;
	std::stringstream insertvalues;
	std::stringstream insertadd;
	std::stringstream insertparams;
	std::stringstream load;
	std::stringstream loadadd;
	std::string autoincrement;
	insert << "\"INSERT INTO " << table.name << " (";
	int insertValues = 0;
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		if (f.isPrimaryKey()) {
			const std::string& cpptype = getCPPType(f.type, true);
			if (primaryKeys > 0) {
				src << ", ";
				load << " AND ";
			} else {
				load << "\"SELECT * FROM " << table.name << " WHERE ";
				src << "\tbool selectById(";
			}
			++primaryKeys;
			load << f.name << " = ";
			sep(load, primaryKeys);
			loadadd << ".add(" << f.name << ")";
			src << cpptype << " " << f.name;
		}
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
		insert << " RETURNING " << autoincrement;
	}
	insert << "\"";
	if (primaryKeys > 0) {
		load << "\"";
		src << ") {\n";
		src << "\t\tSuper::PreparedStatement __p_ = prepare(\"" << classname << "Load\",\n\t\t\t"  << load.str() << ");\n";
		src << "\t\t__p_" << loadadd.str() << ";\n";
		src << "\t\tconst State& __state = __p_.exec();\n";
		src << "\t\tcore_assert_msg(__state.result, \"Failed to execute selectById statement - error: '%s'\", __state.lastErrorMsg.c_str());\n";
		src << "\t\treturn __state.result;\n\t}\n\n";
	}

	std::stringstream createTable;

	src << "\t/**\n";
	src << "\t * @brief Insert a new row into the database with the given parameters\n";
	src << "\t * @note Also fills the generated keys in the model instance\n";
	src << "\t * @return @c true if the execution was successful, @c false otherwise\n";
	src << "\t */\n";
	src << "\tbool insert(";
	src << insertparams.str();
	src << ") {\n";
	src << "\t\tSuper::PreparedStatement __p_ = prepare(\"" << classname << "Insert\",\n\t\t\t"  << insert.str() << ");\n";
	src << "\t\t__p_" << insertadd.str() << ";\n";
	src << "\t\treturn __p_.exec().result;\n";
	src << "\t}\n\n";

	src << "\tstatic bool truncate() {\n";
	src << "\t\treturn " << classname << "().exec(\"TRUNCATE TABLE " << table.name << ";\");\n";
	src << "\t}\n\n";

	createTable << "CREATE TABLE IF NOT EXISTS " << table.name << " (\"\n";
	bool firstField = true;
	for (auto entry : table.fields) {
		const persistence::Model::Field& f = entry.second;
		const std::string& cpptype = getCPPType(f.type, true, isPointer(f));
		std::string n = f.name;
		core::string::upperCamelCase(n);

		if (!firstField) {
			createTable << ",\"\n";
		}
		createTable << "\t\t\t\"" << f.name << " " << getDbType(f) << getDbFlags(table, f);

		src << "\tinline " << cpptype << " " << f.name << "() const {\n";
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

		src << "\tinline void set" << n << "(" << cpptype << " " << f.name << ") {\n";
		src << "\t\t_m._" << f.name << " = " << f.name << ";\n";
		src << "\t}\n\n";

		firstField = false;
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
	src << "\t\treturn " << classname << "().exec(\"" << createTable.str() << "\");\n";
	src << "\t}\n";

	src << "}; // class " << classname << "\n\n";
	src << "} // namespace ::persistence\n\n";
	if (!table.namespaceSrc.empty()) {
		src << "} // namespace " << table.namespaceSrc << "\n\n";
	}

	return true;
}

bool DatabaseTool::generateSrc() const {
	Log::info("Generate database bindings for %s", _targetFile.c_str());
	std::stringstream src;
	src << "#pragma once\n";
	src << "\n";
	src << "#include \"persistence/Model.h\"\n";
	src << "#include \"core/String.h\"\n\n";
	src << "#include \"core/Common.h\"\n\n";

	for (auto i : _tables) {
		const Table& table = i.second;
		if (!generateClassForTable(table, src)) {
			return false;
		}
	}

	return core::App::getInstance()->filesystem()->syswrite(_targetFile, src.str());
}

bool DatabaseTool::parseField(core::Tokenizer& tok, Table& table) const {
	if (!tok.hasNext()) {
		Log::error("Expected field name");
		return false;
	}
	const std::string& fieldname = tok.next();
	if (!tok.hasNext()) {
		Log::error("Expected { after field name %s", fieldname.c_str());
		return false;
	}
	std::string token = tok.next();
	if (token != "{") {
		Log::error("Expected {, found %s", token.c_str());
		return false;
	}
	persistence::Model::Field field;
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
			persistence::Model::FieldType typeMapping = persistence::Model::FieldType::STRING;
			bool foundType = false;
			for (int i = 0; i < persistence::Model::MAX_FIELDTYPES; ++i) {
				if (core::string::iequals(type, FieldTypeNames[i])) {
					typeMapping = (persistence::Model::FieldType)i;
					foundType = true;
					break;
				}
			}
			if (!foundType) {
				Log::error("invalid field type for field %s: %s", fieldname.c_str(), type.c_str());
				return false;
			}
			field.type = typeMapping;
		} else if (token == "notnull") {
			auto i = table.contraints.find(fieldname);
			if (i != table.contraints.end()) {
				Constraint& c = i->second;
				c.types |= std::enum_value(persistence::Model::ConstraintType::NOTNULL);
			} else {
				table.contraints.insert(std::make_pair(fieldname, Constraint{fieldname, (uint32_t)std::enum_value(persistence::Model::ConstraintType::NOTNULL)}));
			}
		} else if (token == "default") {
			if (!tok.hasNext()) {
				Log::error("Missing value for default of %s", fieldname.c_str());
				return false;
			}
			if (!field.defaultVal.empty()) {
				Log::error("There is already a default value (%s) defined for field '%s'", field.defaultVal.c_str(), fieldname.c_str());
				return false;
			}
			field.defaultVal = tok.next();
		} else if (token == "length") {
			if (!tok.hasNext()) {
				Log::error("Missing value for length of '%s'", fieldname.c_str());
				return false;
			}
			if (field.type != persistence::Model::FieldType::STRING && field.type != persistence::Model::FieldType::PASSWORD) {
				Log::error("Field '%s' of type %i doesn't support length parameter", fieldname.c_str(), std::enum_value(field.type));
				return false;
			}
			field.length = core::string::toInt(tok.next());
		} else {
			Log::error("Unknown token found in table definition: %s", token.c_str());
			return false;
		}
	}
	table.fields.insert(std::make_pair(fieldname, field));
	return true;
}

bool DatabaseTool::parseConstraints(core::Tokenizer& tok, Table& table) const {
	if (!tok.hasNext()) {
		Log::error("Expected { after constraints");
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
		if (!tok.hasNext()) {
			Log::error("missing type for field constraint %s", token.c_str());
			return false;
		}
		const std::string& type = tok.next();
		uint32_t typeMapping = 0u;
		for (uint32_t i = 0; i < persistence::Model::MAX_CONSTRAINTTYPES; ++i) {
			if (core::string::iequals(type, ConstraintTypeNames[i])) {
				typeMapping = 1 << i;
				break;
			}
		}
		if (typeMapping == 0u) {
			Log::error("invalid field type for field constraint %s: %s", token.c_str(), type.c_str());
			return false;
		}
		auto i = table.contraints.find(token);
		if (i != table.contraints.end()) {
			Constraint& c = i->second;
			c.types |= typeMapping;
		} else {
			table.contraints.insert(std::make_pair(token, Constraint{token, typeMapping}));
		}
	}
	return true;
}

bool DatabaseTool::parseTable(core::Tokenizer& tok, Table& table) const {
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

	for (auto entry : table.contraints) {
		const Constraint& c = entry.second;
		if (table.fields.find(c.field) == table.fields.end()) {
			Log::error("constraint referenced field wasn't found: %s", c.field.c_str());
			return false;
		}
		Log::debug("transfer constraint to field for faster lookup for %s", c.field.c_str());
		table.fields[c.field].contraintMask |= c.types;
		if ((c.types & std::enum_value(persistence::Model::ConstraintType::PRIMARYKEY)) != 0) {
			++table.primaryKeys;
		}
	}

	return !table.fields.empty();
}

bool DatabaseTool::parse(const std::string& buffer) {
	core::Tokenizer tok(buffer, " ");
	while (tok.hasNext()) {
		const std::string& token = tok.next();
		if (token == "table") {
			if (!tok.hasNext()) {
				Log::error("Expected table name");
				return false;
			}
			const std::string& tablename = tok.next();
			Table table;
			table.name = tablename;
			table.classname = core::string::upperCamelCase(tablename + "Model");
			if (!parseTable(tok, table)) {
				return false;
			}
			_tables.insert(std::make_pair(tablename, table));
		}
	}
	return true;
}

core::AppState DatabaseTool::onConstruct() {
	registerArg("--tablefile").setShort("-t").setDescription("The path to the table to file").setMandatory();
	registerArg("--outfile").setShort("-o").setDescription("The file that should be generated").setMandatory();
	return Super::onConstruct();
}


core::AppState DatabaseTool::onRunning() {
	_tableFile    = getArgVal("--tablefile");
	_targetFile   = getArgVal("--outfile");

	Log::debug("Preparing table file %s", _tableFile.c_str());
	const std::string& buf = filesystem()->load(_tableFile);
	if (buf.empty()) {
		Log::error("Could not load %s", _tableFile.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	if (!parse(buf.c_str())) {
		_exitCode = 1;
		return core::AppState::Cleanup;
	}
	if (!generateSrc()) {
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	return core::AppState::Cleanup;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	DatabaseTool app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
