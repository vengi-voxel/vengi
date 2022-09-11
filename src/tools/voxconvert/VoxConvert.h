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
	core::VarPtr _scaleX;
	core::VarPtr _scaleY;
	core::VarPtr _scaleZ;
	core::VarPtr _quads;
	core::VarPtr _withColor;
	core::VarPtr _withTexCoords;

	bool _mergeVolumes = false;
	bool _scaleVolumes = false;
	bool _mirrorVolumes = false;
	bool _rotateVolumes = false;
	bool _translateVolumes = false;
	bool _exportPalette = false;
	bool _exportLayers = false;
	bool _cropVolumes = false;
	bool _splitVolumes = false;
	bool _dumpSceneGraph = false;
	bool _resizeVolumes = false;

protected:
	glm::ivec3 getArgIvec3(const core::String &name);
	core::String getFilenameForLayerName(const core::String& inputfile, const core::String &layerName, int id);
	bool handleInputFile(const core::String &infile, voxelformat::SceneGraph &sceneGraph, bool multipleInputs);

	void usage() const override;
	void mirror(const core::String& axisStr, voxelformat::SceneGraph& sceneGraph);
	void rotate(const core::String& axisStr, voxelformat::SceneGraph& sceneGraph);
	void scale(voxelformat::SceneGraph& sceneGraph);
	void resize(const glm::ivec3 &size, voxelformat::SceneGraph& sceneGraph);
	void script(const core::String &scriptParameters, voxelformat::SceneGraph& sceneGraph, uint8_t color);
	void translate(const glm::ivec3& pos, voxelformat::SceneGraph& sceneGraph);
	void crop(voxelformat::SceneGraph& sceneGraph);
	int dumpNode_r(const voxelformat::SceneGraph& sceneGraph, int nodeId, int indent);
	void dump(const voxelformat::SceneGraph& sceneGraph);
	void filterVolumes(voxelformat::SceneGraph& sceneGraph);
	void exportLayersIntoSingleObjects(voxelformat::SceneGraph& sceneGraph, const core::String &inputfile);
	void split(const glm::ivec3 &size, voxelformat::SceneGraph& sceneGraph);
public:
	VoxConvert(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
};
