/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "core/collection/StringSet.h"

/**
 * @ingroup Tools
 */
class FormatPrinter : public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;
	void printJson(bool palette, bool image, bool voxel);
	void printMimeInfo();
	void printMarkdownTables();
	void printManPageLoadSaveFormats();
	void printApplicationPlist();

	core::StringSet _uniqueMimetypes;
	core::String uniqueMimetype(const io::FormatDescription &desc);

public:
	FormatPrinter(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onConstruct() override;
	app::AppState onRunning() override;
};
