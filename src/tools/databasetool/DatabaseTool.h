/**
 * @file
 */

#pragma once

#include <map>
#include "core/CommandlineApp.h"
#include "core/Tokenizer.h"
#include "Table.h"

/**
 * @brief This tool will generate c++ code for *.tbl files. These files are a meta description of
 * database tables.
 *
 * @ingroup Tools
 */
class DatabaseTool: public core::CommandlineApp {
private:
	using Super = core::CommandlineApp;
protected:
	std::string _tableFile;
	std::string _targetFile;

	typedef std::map<std::string, databasetool::Table> Tables;
	Tables _tables;

	bool validateForeignKeys(const databasetool::Table& table) const;
	bool validateOperators(const databasetool::Table& table) const;
	bool validate() const;
	bool parse(const std::string& src);
	bool generateSrc() const;
public:
	DatabaseTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onConstruct() override;
	core::AppState onRunning() override;
};
