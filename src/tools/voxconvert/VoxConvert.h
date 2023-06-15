/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "scenegraph/SceneGraph.h"

/**
 * @brief This tool is able to convert voxel volumes between different formats
 *
 * @ingroup Tools
 */
class VoxConvert: public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;
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

	bool _mergeModels = false;
	bool _scaleModels = false;
	bool _mirrorModels = false;
	bool _rotateModels = false;
	bool _translateModels = false;
	bool _exportPalette = false;
	bool _exportModels = false;
	bool _cropModels = false;
	bool _surfaceOnly = false;
	bool _splitModels = false;
	bool _dumpSceneGraph = false;
	bool _dumpMeshDetails = false;
	bool _resizeModels = false;

	struct NodeStats {
		int voxels = 0;
		int vertices = 0;
		int indices = 0;

		NodeStats operator+ (const NodeStats& other) const {
			NodeStats result;
			result.voxels = voxels + other.voxels;
			result.vertices = vertices + other.vertices;
			result.indices = indices + other.indices;
			return result;
		}

		NodeStats &operator+= (const NodeStats& other) {
			voxels += other.voxels;
			vertices += other.vertices;
			indices += other.indices;
			return *this;
		}
	};
protected:
	glm::ivec3 getArgIvec3(const core::String &name);
	core::String getFilenameForModelName(const core::String& inputfile, const core::String &modelName, int id);
	bool handleInputFile(const core::String &infile, scenegraph::SceneGraph &sceneGraph, bool multipleInputs);

	void usage() const override;
	void mirror(const core::String& axisStr, scenegraph::SceneGraph& sceneGraph);
	void rotate(const core::String& axisStr, scenegraph::SceneGraph& sceneGraph);
	void scale(scenegraph::SceneGraph& sceneGraph);
	void resize(const glm::ivec3 &size, scenegraph::SceneGraph& sceneGraph);
	void script(const core::String &scriptParameters, scenegraph::SceneGraph& sceneGraph, uint8_t color);
	void translate(const glm::ivec3& pos, scenegraph::SceneGraph& sceneGraph);
	void crop(scenegraph::SceneGraph& sceneGraph);
	void removeNonSurfaceVoxels(scenegraph::SceneGraph& sceneGraph);
	NodeStats dumpNode_r(const scenegraph::SceneGraph& sceneGraph, int nodeId, int indent, bool meshDetails);
	void dump(const scenegraph::SceneGraph& sceneGraph);
	void dumpMeshDetails(const scenegraph::SceneGraph& sceneGraph);
	void filterModels(scenegraph::SceneGraph& sceneGraph);
	void exportModelsIntoSingleObjects(scenegraph::SceneGraph& sceneGraph, const core::String &inputfile);
	void split(const glm::ivec3 &size, scenegraph::SceneGraph& sceneGraph);
public:
	VoxConvert(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
};
