/**
 * @file
 */

#pragma once

#include "ui/UIApp.h"

/**
 * @brief This tool provides a UI to create noise images on-the-fly.
 */
class VoxEdit: public ui::UIApp {
private:
	using Super = ui::UIApp;
	core::VarPtr _lastDirectory;

public:
	VoxEdit(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	bool saveFile(std::string_view file);
	bool loadFile(std::string_view file);
	bool newFile(bool force = false);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
};
