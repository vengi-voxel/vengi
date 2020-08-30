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
class UITool: public core::CommandlineApp {
private:
	using Super = core::CommandlineApp;
	DummyRenderer _renderer;
	tb::TBWidget _root;
public:
	UITool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
};
