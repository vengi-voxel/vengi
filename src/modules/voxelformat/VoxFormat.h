/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/DynamicArray.h"
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
 * z is pointing upwards
 *
 * https://github.com/ephtracy/voxel-model.git
 * https://ephtracy.github.io/
 */
class VoxFormat : public Format {
private:
	struct ChunkHeader {
		uint32_t chunkId;
		uint32_t numBytesChunk;
		uint32_t numBytesChildrenChunks;
		uint32_t nextChunkPos;
	};

	struct RotationMatrixPacked {
		uint8_t nonZeroEntryInFirstRow : 2;
		uint8_t nonZeroEntryInSecondRow : 2;
		uint8_t signInFirstRow : 1;
		uint8_t signInSecondRow : 1;
		uint8_t signInThirdRow : 1;
		uint8_t unused : 1;
	};
	static_assert(sizeof(RotationMatrixPacked) == 1, "packed rotation matrix should be 1 byte");

	using Attributes = core::StringMap<core::String>;
	using NodeId = uint32_t;

	struct VoxTransform {
		glm::quat rotation = glm::quat_cast(glm::mat3(1.0f));
		glm::ivec3 translation { 0 };
		int32_t layerId;
		uint32_t numFrames;
	};

	struct VoxModel {
		// node id in the scene graph
		NodeId nodeId = 0;
		// there can be multiple SIZE and XYZI chunks for multiple models; volume id is their index in the
		// stored order and the index in the @c _models or @c _regions arrays
		uint32_t volumeIdx = 0;
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

	struct State {
		// index here is the node id
		core::Map<NodeId, SceneGraphNode> _sceneGraphMap;
		uint32_t _volumeIdx = 0u;
		uint32_t _chunks = 0u;
		uint32_t _numModels = 1u;
		core::DynamicArray<Region> _regions;
		core::DynamicArray<VoxModel> _models;
		bool _foundSceneGraph = false;
		core::DynamicArray<VoxTransform> _transforms;
		core::Map<NodeId, NodeId> _parentNodes;
		core::DynamicArray<NodeId> _leafNodes;
	};

	bool skipSaving(const VoxelVolume& v) const;
	bool saveAttributes(const Attributes& attributes, io::SeekableWriteStream& stream) const;

	bool saveChunk_LAYR(State& state, io::SeekableWriteStream& stream, int modelId, const core::String& name, bool visible);
	bool saveChunk_XYZI(State& state, io::SeekableWriteStream& stream, const voxel::RawVolume* volume, const voxel::Region& region);
	bool saveChunk_SIZE(State& state, io::SeekableWriteStream& stream, const voxel::Region& region);
	bool saveChunk_PACK(State& state, io::SeekableWriteStream& stream, const VoxelVolumes& volumes);
	bool saveChunk_RGBA(State& state, io::SeekableWriteStream& stream);

	// scene graph saving stuff
	bool saveChunk_nGRP(State& state, io::SeekableWriteStream& stream, NodeId nodeId, uint32_t childNodes);
	bool saveChunk_nSHP(State& state, io::SeekableWriteStream& stream, NodeId nodeId, uint32_t volumeId);
	bool saveChunk_nTRN(State& state, io::SeekableWriteStream& stream, NodeId nodeId, NodeId childNodeId, const glm::ivec3& mins, int layerId);
	bool saveSceneGraph(State& state, io::SeekableWriteStream& stream, const VoxelVolumes& volumes, int modelCount);

	void initPalette();
	void reset();

	bool readChunkHeader(io::SeekableReadStream& stream, ChunkHeader& header) const;
	bool readAttributes(Attributes& attributes, io::SeekableReadStream& stream) const;

	bool checkVersionAndMagic(io::SeekableReadStream& stream) const;
	bool checkMainChunk(io::SeekableReadStream& stream) const;

	// first iteration
	bool loadChunk_MATL(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadChunk_MATT(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadChunk_IMAP(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadChunk_NOTE(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadChunk_PACK(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadChunk_RGBA(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadChunk_rOBJ(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadChunk_rCAM(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadChunk_SIZE(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadFirstChunks(State& state, io::SeekableReadStream& stream);

	// second iteration
	bool loadChunk_LAYR(State& state, io::SeekableReadStream& stream, const ChunkHeader& header, VoxelVolumes& volumes);
	bool loadChunk_XYZI(State& state, io::SeekableReadStream& stream, const ChunkHeader& header, VoxelVolumes& volumes);
	bool loadSecondChunks(State& state, io::SeekableReadStream& stream, VoxelVolumes& volumes);

	// scene graph
	bool parseSceneGraphTranslation(VoxTransform& transform, const Attributes& attributes) const;
	bool parseSceneGraphRotation(VoxTransform& transform, const Attributes& attributes) const;
	bool loadChunk_nGRP(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadChunk_nSHP(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadChunk_nTRN(State& state, io::SeekableReadStream& stream, const ChunkHeader& header);
	bool loadSceneGraph(State& state, io::SeekableReadStream& stream);
	VoxTransform calculateTransform(State& state, uint32_t volumeIdx) const;
	bool applyTransform(State& state, VoxTransform& transform, NodeId nodeId) const;
	glm::ivec3 calcTransform(State& state, const VoxTransform& t, int x, int y, int z, const glm::ivec3& pivot) const;

public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, core::Array<uint32_t, 256> &palette) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
