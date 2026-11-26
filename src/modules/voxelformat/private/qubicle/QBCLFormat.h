/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include "math/Vec3u.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Qubicle project file (qbcl) format.
 *
 * https://gist.github.com/tostc/7f049207a2e5a7ccb714499702b5e2fd
 *
 * @see QBTFormat
 * @see QBFormat
 * @see QEFFormat
 *
 * @ingroup Formats
 */
class QBCLFormat : public Format {
private:
	struct Header {
		uint32_t magic = 0;
		uint32_t version = 0; // (major, minor, release, build)
		uint32_t fileVersion = 0;
		uint32_t thumbWidth = 0;
		uint32_t thumbHeight = 0;
		core::String title;
		core::String desc;
		core::String metadata;
		core::String author;
		core::String company;
		core::String website;
		core::String copyright;
		uint64_t timestamp1;
		uint64_t timestamp2;
	};
	struct NodeHeader {
		bool visible;
		bool unknown;
		bool locked;
	};
	bool saveMatrix(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
					const scenegraph::SceneGraphNode &node) const;
	bool saveModel(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
				   const scenegraph::SceneGraphNode &node) const;
	bool saveNode(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
				  const scenegraph::SceneGraphNode &node) const;
	bool saveCompound(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
					  const scenegraph::SceneGraphNode &node) const;

	bool readHeader(io::SeekableReadStream &stream, Header &header);
	template<class Visitor>
	bool readVoxels(io::SeekableReadStream &stream, const math::Vec3u &size, uint32_t compressedSize, Visitor &&visit);
	bool readMatrix(io::SeekableReadStream &stream, scenegraph::SceneGraph *sceneGraph, int parent,
					const core::String &name, const NodeHeader &nodeHeader, palette::RGBABuffer *colors);
	bool readNodes(io::SeekableReadStream &stream, scenegraph::SceneGraph *sceneGraph, int parent,
				   palette::RGBABuffer *colors, const LoadContext &ctx);
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
								   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Qubicle Project",
									"",
									{"qbcl"},
									{"QBCL"},
									VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED | VOX_FORMAT_FLAG_PALETTE_EMBEDDED |
										FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
