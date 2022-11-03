/**
 * @file
 */

#pragma once

#include "Format.h"
#include "voxelformat/BinVoxFormat.h"

namespace voxelformat {

/**
 * @brief Qubicle Binary Tree (qbt) is the successor of the widespread voxel exchange format Qubicle Binary. It supports palette
 * and RGBA mode
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
	enum class ColorFormat : uint8_t {
		RGBA,
		Palette
	};
	struct Header {
		uint8_t versionMajor = 0;
		uint8_t versionMinor = 0;
		ColorFormat colorFormat = ColorFormat::RGBA;
		glm::vec3 globalScale {0};
	};

	bool loadHeader(io::SeekableReadStream& stream, Header &state);

	bool skipNode(io::SeekableReadStream& stream);
	bool loadMatrix(io::SeekableReadStream& stream, SceneGraph &sceneGraph, int parent, voxel::Palette &palette, Header &state);
	bool loadCompound(io::SeekableReadStream& stream, SceneGraph &sceneGraph, int parent, voxel::Palette &palette, Header &state);
	bool loadModel(io::SeekableReadStream& stream, SceneGraph &sceneGraph, int parent, voxel::Palette &palette, Header &state);
	bool loadNode(io::SeekableReadStream& stream, SceneGraph &sceneGraph, int parent, voxel::Palette &palette, Header &state);
	bool loadColorMap(io::SeekableReadStream& stream, voxel::Palette &palette);
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;

	bool saveNode(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, const SceneGraphNode& node, bool colorMap) const;
	bool saveCompound(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, const SceneGraphNode& node, bool colorMap) const;
	bool saveMatrix(io::SeekableWriteStream& stream, const SceneGraphNode& node, bool colorMap) const;
	bool saveColorMap(io::SeekableWriteStream& stream, const voxel::Palette& palette) const;
	bool saveModel(io::SeekableWriteStream& stream, const SceneGraph &sceneGraph, const SceneGraphNode& node, bool colorMap) const;
	bool saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, ThumbnailCreator thumbnailCreator) override;
public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) override;
};

}
