/**
 * @file
 */

#pragma once

#include "ui/turbobadger/UIApp.h"

class TestTurbobadger: public ui::turbobadger::UIApp {
private:
	using Super = ui::turbobadger::UIApp;
public:
	TestTurbobadger(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit();
};
