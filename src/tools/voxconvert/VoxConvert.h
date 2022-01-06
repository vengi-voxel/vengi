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
	void mirror(const core::String& axisStr, voxel::SceneGraph& sceneGraph);
	void rotate(const core::String& axisStr, voxel::SceneGraph& sceneGraph);
	void scale(voxel::SceneGraph& sceneGraph);
	void script(const core::String &scriptParameters, voxel::SceneGraph& sceneGraph);
	void translate(const glm::ivec3& pos, voxel::SceneGraph& sceneGraph);
	void pivot(const glm::ivec3& pivot, voxel::SceneGraph& sceneGraph);
	void crop(voxel::SceneGraph& sceneGraph);
	void filterVolumes(voxel::SceneGraph& sceneGraph);
	void split(const glm::ivec3 &size, voxel::SceneGraph& sceneGraph);
public:
	VoxConvert(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
};
