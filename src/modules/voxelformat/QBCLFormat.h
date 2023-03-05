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
		uint64_t timestamp1;
		uint64_t timestamp2;
		bool loadPalette = false;
	};
	struct NodeHeader {
		bool visible;
		bool unknown;
		bool locked;
	};
	bool saveMatrix(io::SeekableWriteStream& stream, const SceneGraphNode& node) const;
	bool saveModel(io::SeekableWriteStream& stream, const SceneGraph &sceneGraph, const SceneGraphNode& node) const;
	bool saveNode(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, const SceneGraphNode& node) const;
	bool saveCompound(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, const SceneGraphNode& node) const;

	bool readHeader(io::SeekableReadStream& stream, Header &header);
	bool readMatrix(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, Header &header, const NodeHeader &nodeHeader);
	bool readModel(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, Header &header, const NodeHeader &nodeHeader);
	bool readCompound(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, Header &header, const NodeHeader &nodeHeader);
	bool readNodes(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, voxel::Palette &palette, Header &header);
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, const voxel::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) override;
public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette, const LoadContext &ctx) override;
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream, const LoadContext &ctx) override;
};

}
