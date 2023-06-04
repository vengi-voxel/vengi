/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/Set.h"

struct ogt_vox_scene;

namespace voxelformat {

struct ogt_SceneContext;

/**
 * @brief MagicaVoxel vox format load and save functions
 *
 * z is pointing upwards
 *
 * @li https://github.com/ephtracy/voxel-model.git
 * @li https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox-extension.txt
 * @li https://ephtracy.github.io/
 *
 * @ingroup Formats
 */
class VoxFormat : public PaletteFormat {
private:
	glm::ivec3 maxSize() const override;

	void loadCameras(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph);
	int findClosestPaletteIndex(const voxel::Palette &palette);
	bool loadInstance(const ogt_vox_scene *scene, uint32_t ogt_instanceIdx, scenegraph::SceneGraph &sceneGraph,
					  int parent, const glm::mat4 &zUpMat, const voxel::Palette &palette, bool groupHidden = false);
	bool loadGroup(const ogt_vox_scene *scene, uint32_t ogt_parentGroupIdx, scenegraph::SceneGraph &sceneGraph,
				   int parent, const glm::mat4 &zUpMat, core::Set<uint32_t> &addedInstances,
				   const voxel::Palette &palette);
	void loadPalette(const ogt_vox_scene *scene, voxel::Palette &palette);
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;

	void saveNode(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, ogt_SceneContext &ctx,
				  uint32_t parentGroupIdx, uint32_t layerIdx, const voxel::Palette &palette, uint8_t replacement);
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;

public:
	VoxFormat();
	size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
					   const LoadContext &ctx) override;
};

} // namespace voxelformat
