/**
 * @file
 */

#pragma once

#include "io/Archive.h"
#include "voxelformat/Format.h"
#include "ThingNodeParser.h"

namespace core {
class Tokenizer;
}

namespace voxelformat {

/**
 * @brief rooms.xyz (thing)
 *
 * This is a wrapper around the magicavoxel format that adds further information to the nodes with a text file.
 * @sa VoxFormat
 *
 * @todo some thing files contain a icon.png (128x128) thumbnail, which could be used for the thumbnailer
 *
 * https://medium.com/@btco_code/programming-in-rooms-xyz-part-1-cb498b2b4301
 *
 * +Z points to the left wall, +X points to the right wall.
 * north is towards +Z
 * east is towards +X
 * south is towards -Z
 * west is towards -X
 * up is towards +Y
 * down is towards -Y
 *
 * @ingroup Formats
 */
class ThingFormat : public Format {
private:
	bool loadNodeSpec(io::SeekableReadStream &stream, NodeSpec &nodeSpec) const;
	bool addMediaImage(const io::ArchivePtr &archive, const NodeSpec &nodeSpec, scenegraph::SceneGraph &sceneGraph, int parent);
	bool loadNode(const io::ArchivePtr &archive, const NodeSpec &nodeSpec, scenegraph::SceneGraph &sceneGraph,
				  const LoadContext &ctx, int parent = 0);

public:
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Rooms.xyz Thing", "", {"thing"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
