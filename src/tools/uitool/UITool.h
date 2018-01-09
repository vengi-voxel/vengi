/**
 * @file
 */

#pragma once

#include "core/ConsoleApp.h"
#include "ui/turbobadger/TurboBadger.h"
#include "ui/turbobadger/UIDummies.h"

/**
 * @brief This tool validates the turbobadger ui files (*.tb.txt)
 */
class UITool: public core::ConsoleApp {
private:
	using Super = core::ConsoleApp;
	DummyRenderer _renderer;
	tb::TBWidget _root;
public:
	UITool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
};
