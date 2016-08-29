/**
 * @file
 */

#pragma once

#include "core/App.h"
#include "ui/TurboBadger.h"
#include "ui/UIDummies.h"

/**
 * @brief This tool validates te ui files (*.tb.txt)
 */
class UITool: public core::App {
private:
	DummyRenderer _renderer;
	tb::TBWidget _root;
public:
	UITool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
};
