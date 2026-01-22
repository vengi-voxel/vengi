/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"
#include "core/collection/Set.h"

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
protected:
	glm::ivec3 maxSize() const override;
	int emptyPaletteIndex() const override;
private:
	void saveInstance(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, MVSceneContext &ctx,
					 uint32_t parentGroupIdx, uint32_t layerIdx, uint32_t modelIdx);
	bool loadScene(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette);
	bool loadInstance(const ogt_vox_scene *scene, uint32_t ogt_instanceIdx, scenegraph::SceneGraph &sceneGraph,
					  int parent, core::DynamicArray<MVModelToNode> &models, const palette::Palette &palette);
	bool loadGroup(const ogt_vox_scene *scene, uint32_t ogt_parentGroupIdx, scenegraph::SceneGraph &sceneGraph,
				   int parent, core::DynamicArray<MVModelToNode> &models, core::Set<uint32_t> &addedInstances,
				   const palette::Palette &palette);
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;

	void saveNode(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, MVSceneContext &ctx,
				  uint32_t parentGroupIdx, uint32_t layerIdx);
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
public:
	VoxFormat();
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"MagicaVoxel", "", {"vox"}, {"VOX "}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
