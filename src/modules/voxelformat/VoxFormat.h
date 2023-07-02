/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/Set.h"
#include "scenegraph/SceneGraphNode.h"

struct ogt_vox_scene;

namespace voxelformat {

struct MVSceneContext;
struct MVModelToNode;

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
	glm::ivec3 maxSize() const override {
		return glm::ivec3(256);
	}

	void saveInstance(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, MVSceneContext &ctx,
					 uint32_t parentGroupIdx, uint32_t layerIdx);
	bool loadScene(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph, const voxel::Palette &palette);
	bool loadInstance(const ogt_vox_scene *scene, uint32_t ogt_instanceIdx, scenegraph::SceneGraph &sceneGraph,
					  int parent, core::DynamicArray<MVModelToNode> &models, const voxel::Palette &palette);
	bool loadGroup(const ogt_vox_scene *scene, uint32_t ogt_parentGroupIdx, scenegraph::SceneGraph &sceneGraph,
				   int parent, core::DynamicArray<MVModelToNode> &models, core::Set<uint32_t> &addedInstances,
				   const voxel::Palette &palette);
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;

	void saveNode(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, MVSceneContext &ctx,
				  uint32_t parentGroupIdx, uint32_t layerIdx, const voxel::Palette &palette, uint8_t replacement);
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;

public:
	VoxFormat();
	size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
					   const LoadContext &ctx) override;
};

} // namespace voxelformat
