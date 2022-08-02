/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/Set.h"

struct ogt_vox_scene;

namespace voxelformat {

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
	int findClosestPaletteIndex(const voxel::Palette &palette);
	bool addInstance(const ogt_vox_scene *scene, uint32_t ogt_instanceIdx, SceneGraph &sceneGraph, int parent, const glm::mat4 &zUpMat, const voxel::Palette &palette, bool groupHidden = false);
	bool addGroup(const ogt_vox_scene *scene, uint32_t ogt_parentGroupIdx, SceneGraph &sceneGraph, int parent, const glm::mat4 &zUpMat, core::Set<uint32_t> &addedInstances, const voxel::Palette &palette);
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;
public:
	VoxFormat();
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
