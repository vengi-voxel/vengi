/**
 * @file
 */

#pragma once

#include "ui/nuklear/LUAUIApp.h"

class TestLUAUI: public nuklear::LUAUIApp {
private:
	using Super = nuklear::LUAUIApp;
public:
	TestLUAUI(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const video::TexturePoolPtr& texturePool);
};
