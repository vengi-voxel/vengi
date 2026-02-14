/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief VoxEdit (Sandbox) (vxr)
 * Transforms - since version 4 or higher the animations are part of a vxa file
 * They are designed to run at 24 fps
 * @sa VXMFormat
 * @sa VXAFormat
 *
 * @ingroup Formats
 */
class VXRFormat : public PaletteFormat {
private:
	bool onlyOnePalette() const override {
		return false;
	}
	bool loadChildVXM(const core::String &vxmPath, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					  scenegraph::SceneGraphNode &node, int version, const LoadContext &ctx);

	bool handleVersion8AndLater(io::SeekableReadStream &stream, scenegraph::SceneGraphNode &node,
								const LoadContext &ctx);
	bool importChild(const core::String &vxmPath, const io::ArchivePtr &archive, io::SeekableReadStream &stream,
					 scenegraph::SceneGraph &sceneGraph, int version, int parent, const LoadContext &ctx);

	bool loadGroupsVersion4AndLater(const core::String &filename, const io::ArchivePtr &archive, io::SeekableReadStream &stream,
									scenegraph::SceneGraph &sceneGraph, int version, const LoadContext &ctx);

	bool loadGroupsVersion3AndEarlier(const core::String &filename, const io::ArchivePtr &archive,
									  io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, int version,
									  const LoadContext &ctx);
	bool importChildVersion3AndEarlier(const core::String &filename, io::SeekableReadStream &stream,
									   scenegraph::SceneGraph &sceneGraph, int version, int parent,
									   const LoadContext &ctx);
	bool saveRecursiveNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
						   const core::String &filename, const io::ArchivePtr &archive, io::SeekableWriteStream &stream,
						   const SaveContext &ctx);
	bool saveNodeProperties(const scenegraph::SceneGraphNode *node, io::SeekableWriteStream &stream);
	bool saveVXA(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
				 const io::ArchivePtr &archive, const core::String &animation, const SaveContext &ctx);
	bool loadVXA(scenegraph::SceneGraph &sceneGraph, const core::String &vxaPath, const io::ArchivePtr &archive, const LoadContext &ctx);
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	bool supportsReferences() const override {
		return true;
	}
	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
								   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Sandbox VoxEdit Hierarchy",
									"",
									{"vxr"},
									{"VXR9", "VXR8", "VXR7", "VXR6", "VXR5", "VXR4", "VXR3", "VXR2", "VXR1"},
									VOX_FORMAT_FLAG_ANIMATION | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
