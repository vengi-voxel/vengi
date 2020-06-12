/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "core/io/FileStream.h"
#include "core/String.h"
#include "core/collection/StringMap.h"
#include "core/collection/Buffer.h"
#include <glm/mat3x3.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

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
	using NodeId = uint32_t;

	struct VoxTransform {
		glm::quat rotation = glm::quat_cast(glm::mat3(1.0f));
		glm::ivec3 translation { 0 };
	};

	struct VoxModel {
		// node id in the scene graph
		NodeId nodeId;
		// there can be multiple SIZE and XYZI chunks for multiple models; volume id is their index in the
		// stored order and the index in the @c _models or @c _regions arrays
		uint32_t volumeIdx;
		Attributes attributes;
		Attributes nodeAttributes;
	};

	enum class SceneGraphNodeType { Transform, Group, Shape };
	using SceneGraphChildNodes = core::Buffer<NodeId>;

	struct SceneGraphNode {
		// the index in the @c _models, @c _regions or _transforms arrays
		uint32_t arrayIdx = 0u;
		SceneGraphNodeType type = SceneGraphNodeType::Transform;
		SceneGraphChildNodes childNodeIds {0};
	};

	// index here is the node id
	core::Map<NodeId, SceneGraphNode> _sceneGraphMap;
	uint32_t _volumeIdx = 0u;
	uint32_t _chunks = 0u;
	uint32_t _numModels = 1u;
	std::vector<Region> _regions;
	std::vector<VoxModel> _models;
	std::vector<VoxTransform> _transforms;
	core::Map<NodeId, NodeId> _parentNodes;
	std::vector<NodeId> _leafNodes;

	bool skipSaving(const VoxelVolume& v) const;
	bool saveAttributes(const Attributes& attributes, io::FileStream& stream) const;

	bool saveChunk_LAYR(io::FileStream& stream, int modelId, const core::String& name, bool visible);
	bool saveChunk_XYZI(io::FileStream& stream, const voxel::RawVolume* volume, const voxel::Region& region);
	bool saveChunk_SIZE(io::FileStream& stream, const voxel::Region& region);
	bool saveChunk_PACK(io::FileStream& stream, const VoxelVolumes& volumes);
	bool saveChunk_RGBA(io::FileStream& stream);

	// scene graph saving stuff
	bool saveChunk_nGRP(io::FileStream& stream, NodeId nodeId, uint32_t volumes);
	bool saveChunk_nSHP(io::FileStream& stream, NodeId nodeId, uint32_t volumeId);
	bool saveChunk_nTRN(io::FileStream& stream, NodeId nodeId, NodeId childNodeId, const glm::ivec3& mins);
	bool saveSceneGraph(io::FileStream& stream, const VoxelVolumes& volumes, int modelCount);

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

	// second iteration
	bool loadChunk_LAYR(io::FileStream& stream, const ChunkHeader& header, VoxelVolumes& volumes);
	bool loadChunk_XYZI(io::FileStream& stream, const ChunkHeader& header, VoxelVolumes& volumes);
	bool loadSecondChunks(io::FileStream& stream, VoxelVolumes& volumes);

	// scene graph
	bool parseSceneGraphTranslation(VoxTransform& transform, const Attributes& attributes) const;
	bool parseSceneGraphRotation(VoxTransform& transform, const Attributes& attributes) const;
	bool loadChunk_nGRP(io::FileStream& stream, const ChunkHeader& header);
	bool loadChunk_nSHP(io::FileStream& stream, const ChunkHeader& header);
	bool loadChunk_nTRN(io::FileStream& stream, const ChunkHeader& header, VoxelVolumes& volumes);
	bool loadSceneGraph(io::FileStream& stream, VoxelVolumes& volumes);
	VoxTransform calculateTransform(uint32_t volumeIdx) const;
	bool applyTransform(VoxTransform& transform, NodeId nodeId) const;
	glm::ivec3 calcTransform(const VoxTransform& t, float x, float y, float z, const glm::vec3& pivot) const;

public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
