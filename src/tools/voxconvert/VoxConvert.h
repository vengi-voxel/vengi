/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "voxelformat/VoxelVolumes.h"

/**
 * @brief This tool is able to convert voxel volumes between different formats
 *
 * @ingroup Tools
 */
class VoxConvert: public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;
	core::VarPtr _palette;
	core::VarPtr _mergeQuads;
	core::VarPtr _reuseVertices;
	core::VarPtr _ambientOcclusion;
	core::VarPtr _scale;
	core::VarPtr _quads;
	core::VarPtr _withColor;
	core::VarPtr _withTexCoords;
protected:
	void usage() const override;
	void mirror(const core::String& axisStr, voxel::VoxelVolumes& volumes);
	void rotate(const core::String& axisStr, voxel::VoxelVolumes& volumes);
	void filterVolumes(voxel::VoxelVolumes& volumes);
public:
	VoxConvert(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
};
