/**
 * @file
 */

#pragma once

#include "Format.h"

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
class QBCLFormat : public RGBAFormat {
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
		uint8_t guid[16] {};
		bool loadPalette = false;
	};
	bool saveMatrix(io::SeekableWriteStream& stream, const SceneGraphNode& node) const;
	bool saveModel(io::SeekableWriteStream& stream, const SceneGraph &sceneGraph) const;

	bool readHeader(io::SeekableReadStream& stream, Header &header);
	bool readMatrix(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, Header &header);
	bool readModel(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, Header &header);
	bool readCompound(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, Header &header);
	bool readNodes(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, voxel::Palette &palette, Header &header);
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, const voxel::Palette &palette) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, ThumbnailCreator thumbnailCreator) override;
public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) override;
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
};

}
