/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "ui/turbobadger/TurboBadger.h"
#include "ui/turbobadger/UIDummies.h"

/**
 * @brief This tool validates the turbobadger ui files (*.tb.txt)
 *
 * @ingroup Tools
 */
class UITool: public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;
	DummyRenderer _renderer;
	tb::TBWidget _root;
public:
	UITool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	app::AppState onInit() override;
	app::AppState onRunning() override;
	app::AppState onCleanup() override;
};
