/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"

/**
 * @brief This tool is able to convert palettes between different formats
 *
 * @ingroup Tools
 */
class PalConvert : public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;

	void usage() const override;
	void printUsageHeader() const override;
	bool handleInputFile(const core::String &infile, const core::String &outfile);

public:
	PalConvert(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
};
