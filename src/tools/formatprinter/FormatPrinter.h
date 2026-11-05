/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "core/collection/StringSet.h"

namespace io {
class WriteStream;
}

/**
 * @ingroup Tools
 */
class FormatPrinter : public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;
	void printJson(bool palette, bool image, bool voxel);
	void printMimeInfo();
	void printMarkdownTables();
	void addManPageOption(const core::String &option, const core::String &description);
	void printManPageVars();
	void printManPageFormats(const core::String& app, bool save);
	void printManPage(const core::String &app);
	void printApplicationPlist();
	void printThumbnailerPlist();
	void printInstallerWix();
	void printMagic();
	void printLuaApiMarkdown();
	void printUtiDeclaration(const io::FormatDescription &desc);
	void printSupportedContentTypes();

	core::StringSet _varsAtStartup;
	core::StringSet _uniqueMimetypes;
	core::String uniqueMimetype(const io::FormatDescription &desc);
	core::String uniqueUti(const io::FormatDescription &desc);

public:
	FormatPrinter(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onConstruct() override;
	app::AppState onRunning() override;
};
