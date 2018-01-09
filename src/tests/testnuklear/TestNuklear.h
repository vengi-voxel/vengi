/**
 * @file
 */

#pragma once

#include "ui/nuklear/NuklearApp.h"

class TestNuklear: public nuklear::NuklearApp {
private:
	using Super = nuklear::NuklearApp;
public:
	TestNuklear(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	bool onRenderUI() override;
};
