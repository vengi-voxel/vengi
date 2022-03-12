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
	core::VarPtr _frame;
	core::VarPtr _quads;
	core::VarPtr _withColor;
	core::VarPtr _withTexCoords;

	bool _mergeVolumes = false;
	bool _scaleVolumes = false;
	bool _mirrorVolumes = false;
	bool _rotateVolumes = false;
	bool _translateVolumes = false;
	bool _srcPalette = false;
	bool _exportPalette = false;
	bool _exportLayers = false;
	bool _changePivot = false;
	bool _cropVolumes = false;
	bool _splitVolumes = false;
	bool _dumpSceneGraph = false;
	bool _resizeVolumes = false;

	int addNodeToSceneGraph(voxel::SceneGraph& sceneGraph, voxel::SceneGraphNode &node, int parent);
	int addSceneGraphNode_r(voxel::SceneGraph& sceneGraph, voxel::SceneGraph &newSceneGraph, voxel::SceneGraphNode &node, int parent);
	int addSceneGraphNodes(voxel::SceneGraph& sceneGraph, voxel::SceneGraph& newSceneGraph, int parent);

protected:
	glm::ivec3 getArgIvec3(const core::String &name);
	core::String getFilenameForLayerName(const core::String& inputfile, const core::String &layerName, int id);
	bool handleInputFile(const core::String &infile, voxel::SceneGraph &sceneGraph, bool multipleInputs);

	void usage() const override;
	void mirror(const core::String& axisStr, voxel::SceneGraph& sceneGraph);
	void rotate(const core::String& axisStr, voxel::SceneGraph& sceneGraph);
	void scale(voxel::SceneGraph& sceneGraph);
	void resize(const glm::ivec3 &size, voxel::SceneGraph& sceneGraph);
	void script(const core::String &scriptParameters, voxel::SceneGraph& sceneGraph);
	void translate(const glm::ivec3& pos, voxel::SceneGraph& sceneGraph);
	void pivot(const glm::ivec3& pivot, voxel::SceneGraph& sceneGraph);
	void crop(voxel::SceneGraph& sceneGraph);
	void dumpNode_r(const voxel::SceneGraph& sceneGraph, int nodeId, int indent);
	void dump(const voxel::SceneGraph& sceneGraph);
	void filterVolumes(voxel::SceneGraph& sceneGraph);
	void exportLayersIntoSingleObjects(voxel::SceneGraph& sceneGraph, const core::String &inputfile);
	void split(const glm::ivec3 &size, voxel::SceneGraph& sceneGraph);
public:
	VoxConvert(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
};
