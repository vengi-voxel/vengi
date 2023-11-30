/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"

/**
 * @ingroup Tools
 */
class FormatPrinter : public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;

public:
	FormatPrinter(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onConstruct() override;
	app::AppState onRunning() override;
};
