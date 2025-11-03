/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "io/Archive.h"
#include "scenegraph/SceneGraph.h"

/**
 * @brief This tool is able to convert voxel volumes between different formats
 *
 * @ingroup Tools
 */
class VoxConvert : public app::CommandlineApp {
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
	bool _outputJson = false;
	bool _outputImage = false;
	bool _resizeModels = false;

protected:
	glm::ivec3 getArgIvec3(const core::String &name);
	core::String getFilenameForModelName(const core::String &inputfile, const core::String &modelName,
										 const core::String &outExt, int id, bool uniqueNames);
	bool handleInputFile(const core::String &infile, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						 bool multipleInputs);

	void applyFilters(scenegraph::SceneGraph &sceneGraph, const core::DynamicArray<core::String> &infiles,
					  const core::DynamicArray<core::String> &outfiles);
	void applyScripts(scenegraph::SceneGraph &sceneGraph);
	void usage() const override;
	void printUsageHeader() const override;
	void mirror(const core::String &axisStr, scenegraph::SceneGraph &sceneGraph);
	void rotate(const core::String &axisStr, scenegraph::SceneGraph &sceneGraph);
	void scale(scenegraph::SceneGraph &sceneGraph);
	void resize(const glm::ivec3 &size, scenegraph::SceneGraph &sceneGraph);
	void script(const core::String &scriptParameters, scenegraph::SceneGraph &sceneGraph, uint8_t color);
	void translate(const glm::ivec3 &pos, scenegraph::SceneGraph &sceneGraph);
	void crop(scenegraph::SceneGraph &sceneGraph);
	void removeNonSurfaceVoxels(scenegraph::SceneGraph &sceneGraph);
	void filterModels(scenegraph::SceneGraph &sceneGraph);
	void filterModelsByProperty(scenegraph::SceneGraph &sceneGraph, const core::String &property,
								const core::String &value);
	void exportModelsIntoSingleObjects(scenegraph::SceneGraph &sceneGraph, const core::String &inputfile,
									   const core::String &ext);
	void split(const glm::ivec3 &size, scenegraph::SceneGraph &sceneGraph);

public:
	VoxConvert(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
};
