/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/Set.h"

struct ogt_vox_scene;

namespace voxel {

/**
 * @brief MagicaVoxel vox format load and save functions
 *
 * z is pointing upwards
 *
 * https://github.com/ephtracy/voxel-model.git
 * https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox-extension.txt
 * https://ephtracy.github.io/
 */
class VoxFormat : public Format {
private:
	int findClosestPaletteIndex();
	bool addInstance(const ogt_vox_scene *scene, uint32_t ogt_instanceIdx, SceneGraph &sceneGraph, int parent, const glm::mat4 &zUpMat, bool groupHidden = false);
	bool addGroup(const ogt_vox_scene *scene, uint32_t ogt_parentGroupIdx, SceneGraph &sceneGraph, int parent, const glm::mat4 &zUpMat, core::Set<uint32_t> &addedInstances);
public:
	VoxFormat();
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, core::Array<uint32_t, 256> &palette) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
