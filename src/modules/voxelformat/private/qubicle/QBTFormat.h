/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Qubicle Binary Tree (qbt) is the successor of the widespread voxel exchange format Qubicle Binary. It supports
 * palette and RGBA mode
 *
 * @see QBCLFormat
 * @see QBFormat
 * @see QEFFormat
 *
 * https://getqubicle.com/qubicle/documentation/docs/file/qbt/
 *
 * @ingroup Formats
 */
class QBTFormat : public PaletteFormat {
private:
	enum class ColorFormat : uint8_t { RGBA, Palette };
	struct Header {
		uint8_t versionMajor = 0;
		uint8_t versionMinor = 0;
		ColorFormat colorFormat = ColorFormat::RGBA;
		glm::vec3 globalScale{0};
	};

	bool loadHeader(io::SeekableReadStream &stream, Header &state);

	bool skipNode(io::SeekableReadStream &stream);
	bool loadMatrix(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, int parent,
					palette::Palette &palette, Header &state);
	bool loadCompound(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, int parent,
					  palette::Palette &palette, Header &state);
	bool loadModel(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, int parent,
				   palette::Palette &palette, Header &state);
	bool loadNode(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, int parent,
				  palette::Palette &palette, Header &state);
	bool loadColorMap(io::SeekableReadStream &stream, palette::Palette &palette);
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;

	bool saveNode(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
				  const scenegraph::SceneGraphNode &node, bool colorMap) const;
	bool saveCompound(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
					  const scenegraph::SceneGraphNode &node, bool colorMap) const;
	bool saveMatrix(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
					const scenegraph::SceneGraphNode &node, bool colorMap) const;
	bool saveColorMap(io::SeekableWriteStream &stream, const palette::Palette &palette) const;
	bool saveModel(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
				   const scenegraph::SceneGraphNode &node, bool colorMap) const;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"Qubicle Binary Tree", "", {"qbt"}, {"QB 2"}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
