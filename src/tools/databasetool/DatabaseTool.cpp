/**
 * @file
 */

#include "DatabaseTool.h"
#include "io/Filesystem.h"
#include "core/String.h"
#include "core/Common.h"
#include "Util.h"
#include "Parser.h"
#include "Mapping.h"
#include "Generator.h"

// TODO:
// * add update and delete methods
// * move sql syntax related methods into vendor specific classes
// * use raw string literals
// * password support
DatabaseTool::DatabaseTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider, 0) {
	init(ORGANISATION, "databasetool");
}

bool DatabaseTool::generateSrc() const {
	Log::debug("Generate database bindings for %s", _targetFile.c_str());
	std::stringstream header;
	header << "#pragma once\n\n";

	const std::string dir(core::string::extractPath(_targetFile.c_str()));
	bool error = false;
	for (auto i : _tables) {
		const databasetool::Table& table = i.second;
		std::stringstream src;
		if (!databasetool::generateClassForTable(table, src)) {
			error = true;
			continue;
		}
		header << "#include \"" << table.classname << ".h\"\n";
		const std::string filename = dir + table.classname + ".h";
		if (!filesystem()->syswrite(filename, src.str())) {
			error = true;
			continue;
		}
	}
	if (error) {
		return false;
	}
	return filesystem()->syswrite(_targetFile, header.str());
}

bool DatabaseTool::parse(const std::string& buffer) {
	core::Tokenizer tok(buffer, " \t\n", "(){},;");
	while (tok.hasNext()) {
		const std::string& token = tok.next();
		if (token == "table") {
			if (!tok.hasNext()) {
				Log::error("Expected table name");
				return false;
			}
			const std::string& tablename = tok.next();
			databasetool::Table table;
			table.name = tablename;
			table.classname = core::string::upperCamelCase(tablename + "Model");
			if (!databasetool::parseTable(tok, table)) {
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
