/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "voxelformat/SceneGraph.h"

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
	glm::ivec3 getArgIvec3(const core::String &name);

	void usage() const override;
	void mirror(const core::String& axisStr, voxel::SceneGraph& volumes);
	void rotate(const core::String& axisStr, voxel::SceneGraph& volumes);
	void scale(voxel::SceneGraph& volumes);
	void script(const core::String &scriptParameters, voxel::SceneGraph& volumes);
	void translate(const glm::ivec3& pos, voxel::SceneGraph& volumes);
	void pivot(const glm::ivec3& pivot, voxel::SceneGraph& volumes);
	void crop(voxel::SceneGraph& volumes);
	void filterVolumes(voxel::SceneGraph& volumes);
	void split(const glm::ivec3 &size, voxel::SceneGraph& volumes);
public:
	VoxConvert(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
};
