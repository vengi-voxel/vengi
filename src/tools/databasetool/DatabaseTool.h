/**
 * @file
 */

#pragma once

#include <map>
#include "app/CommandlineApp.h"
#include "core/Tokenizer.h"
#include "Table.h"

/**
 * @brief This tool will generate c++ code for *.tbl files. These files are a meta description of
 * database tables.
 *
 * @ingroup Tools
 */
class DatabaseTool: public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;
protected:
	core::String _tableFile;
	core::String _targetFile;

	typedef std::map<core::String, databasetool::Table> Tables;
	Tables _tables;

	bool validateForeignKeys(const databasetool::Table& table) const;
	bool validateOperators(const databasetool::Table& table) const;
	bool validate() const;
	bool parse(const core::String& src);
	bool generateSrc() const;
public:
	DatabaseTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onRunning() override;
};
