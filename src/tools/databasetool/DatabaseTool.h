/**
 * @file
 */

#pragma once

#include <map>
#include "core/App.h"
#include "core/Tokenizer.h"
#include "persistence/Model.h"
#include <sstream>

/**
 * @brief This tool will generate c++ code for *.tbl files. These files are a meta description of
 * database tables.
 */
class DatabaseTool: public core::App {
protected:
	std::string _tableFile;
	std::string _targetFile;

	struct Constraint {
		std::string field;
		// bitmask from persistence::Model::ConstraintType
		uint32_t types;
	};

	typedef std::map<std::string, Constraint> Constraints;
	// TODO: sort for insertion order - keep it stable
	typedef std::map<std::string, persistence::Model::Field> Fields;

	struct Table {
		std::string name;
		std::string classname;
		std::string namespaceSrc = "backend";
		Fields fields;
		Constraints contraints;
		int primaryKeys = 0;
	};

	typedef std::map<std::string, Table> Tables;
	Tables _tables;

	bool needsInitCPP(persistence::Model::FieldType type) const;
	std::string getDbType(const persistence::Model::Field& field) const;
	std::string getDbFlags(const Table& table, const persistence::Model::Field& field) const;
	std::string getCPPType(persistence::Model::FieldType type, bool function = false, bool pointer = false) const;
	std::string getCPPInit(persistence::Model::FieldType type) const;

	void sep(std::stringstream& ss, int count) const;

	bool parseConstraints(core::Tokenizer& tok, Table& table) const;
	bool parseField(core::Tokenizer& tok, Table& table) const;
	bool parseTable(core::Tokenizer& tok, Table& table) const;
	bool parse(const std::string& src);
	bool generateClassForTable(const Table& table, std::stringstream& src) const;
	bool generateSrc() const;
public:
	DatabaseTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onRunning() override;
};
