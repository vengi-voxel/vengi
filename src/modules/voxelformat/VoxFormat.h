/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "core/io/FileStream.h"
#include "core/String.h"
#include "core/collection/StringMap.h"

namespace voxel {

/**
 * @brief MagicaVoxel vox format load and save functions
 *
 * https://github.com/ephtracy/voxel-model.git
 * https://voxel.codeplex.com/wikipage?title=Sample%20Codes
 */
class VoxFormat : public VoxFileFormat {
private:
	struct ChunkHeader {
		uint32_t chunkId;
		uint32_t numBytesChunk;
		uint32_t numBytesChildrenChunks;
		uint32_t nextChunkPos;
	};

	using Attributes = core::StringMap<core::String>;

	struct VoxModel {
		uint32_t nodeId;
		uint32_t modelId;
		Attributes attributes;
		Attributes nodeAttributes;
	};

	std::vector<Region> _regions;
	std::vector<VoxModel> _models;
	uint32_t _volumeIdx = 0;

	bool saveAttributes(const Attributes& attributes, io::FileStream& stream) const;
	bool saveChunk_LAYR(io::FileStream& stream, int layerId, const core::String& name, bool visible) const;
	bool saveChunk_nTRN(io::FileStream& stream, int layerId, const voxel::Region& region) const;
	bool saveChunk_XYZI(io::FileStream& stream, const voxel::RawVolume* volume, const voxel::Region& region) const;
	bool saveChunk_SIZE(io::FileStream& stream, const voxel::Region& region) const;
	bool saveChunk_RGBA(io::FileStream& stream) const;

	void initPalette();
	void reset();

	bool readChunkHeader(io::FileStream& stream, ChunkHeader& header) const;
	bool readAttributes(Attributes& attributes, io::FileStream& stream) const;

	bool checkVersionAndMagic(io::FileStream& stream) const;
	bool checkMainChunk(io::FileStream& stream) const;

	// first iteration
	bool loadChunk_MATL(io::FileStream& stream, const ChunkHeader& header);
	bool loadChunk_MATT(io::FileStream& stream, const ChunkHeader& header);
	bool loadChunk_PACK(io::FileStream& stream, const ChunkHeader& header);
	bool loadChunk_RGBA(io::FileStream& stream, const ChunkHeader& header);
	bool loadChunk_rOBJ(io::FileStream& stream, const ChunkHeader& header);
	bool loadChunk_SIZE(io::FileStream& stream, const ChunkHeader& header);
	bool loadFirstChunks(io::FileStream& stream);

	bool loadChunk_LAYR(io::FileStream& stream, const ChunkHeader& header, VoxelVolumes& volumes);
	bool loadChunk_XYZI(io::FileStream& stream, const ChunkHeader& header, VoxelVolumes& volumes);
	bool loadSecondChunks(io::FileStream& stream, VoxelVolumes& volumes);

	bool loadChunk_nGRP(io::FileStream& stream, const ChunkHeader& header);
	bool loadChunk_nSHP(io::FileStream& stream, const ChunkHeader& header);
	bool loadChunk_nTRN(io::FileStream& stream, const ChunkHeader& header, VoxelVolumes& volumes);
	bool loadSceneGraph(io::FileStream& stream, VoxelVolumes& volumes);

public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
