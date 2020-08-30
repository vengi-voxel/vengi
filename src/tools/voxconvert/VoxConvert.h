/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"

/**
 * @brief This tool is able to convert voxel volumes between different formats
 *
 * @ingroup Tools
 */
class VoxConvert: public core::CommandlineApp {
private:
	using Super = core::CommandlineApp;
	core::VarPtr _palette;
public:
	VoxConvert(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onConstruct() override;
	core::AppState onInit() override;
};
