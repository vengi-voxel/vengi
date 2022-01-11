/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/DynamicArray.h"
#include "core/String.h"
#include "core/collection/StringMap.h"
#include "core/collection/Buffer.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraphNode.h"
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
	struct VoxChunkHeader {
		uint32_t chunkId;
		uint32_t numBytesChunk;
		uint32_t numBytesChildrenChunks;
		uint32_t nextChunkPos;
	};

	struct VoxRotationMatrixPacked {
		uint8_t nonZeroEntryInFirstRow : 2;
		uint8_t nonZeroEntryInSecondRow : 2;
		uint8_t signInFirstRow : 1;
		uint8_t signInSecondRow : 1;
		uint8_t signInThirdRow : 1;
		uint8_t unused : 1;
	};
	static_assert(sizeof(VoxRotationMatrixPacked) == 1, "packed rotation matrix should be 1 byte");

	using VoxAttributes = core::StringMap<core::String>;
	using VoxNodeId = uint32_t;

	struct VoxNTRNNode {
		glm::mat3x3 rotMat { 1.0f };
		glm::ivec3 translation { 0 };
		int32_t layerId = 0;
		uint32_t numFrames = 0u;
		VoxAttributes attributes;
		VoxAttributes transformNodeAttributes;
		uint32_t childNodeId = 0u;
		uint32_t reserved = 0u;
	};

	struct VoxLayer {
		uint32_t layerId = 0u;
		VoxAttributes attributes;
		uint32_t modelIdx = 0u;
	};

	struct VoxModel {
		voxel::RawVolume* volume = nullptr;
	};

	struct VoxNSHPNode {
		// node id in the scene graph
		VoxNodeId nodeId = 0;
		// there can be multiple SIZE and XYZI chunks for multiple models; volume id is their index in the
		// stored order and the index in the @c _xyzi or @c _sizes arrays
		int32_t modelId = -1;
		uint32_t shapeNodeNumModels = 0u;
		VoxAttributes modelAttributes;
		VoxAttributes attributes;
	};

	struct VoxROBJ {
		VoxAttributes attributes;
	};

	using VoxSceneGraphChildNodes = core::Buffer<VoxNodeId>;
	struct VoxNGRPNode {
		VoxAttributes attributes;
		VoxSceneGraphChildNodes children;
	};

	struct VoxCamera {
		VoxAttributes attributes;
		uint32_t cameraId;
	};

	enum class VoxSceneGraphNodeType { Transform, Group, Shape };

	struct VoxSceneGraphNode {
		VoxNodeId nodeId = 0u;
		VoxSceneGraphNodeType type = VoxSceneGraphNodeType::Transform;
		VoxSceneGraphChildNodes childNodeIds {0};
	};

	struct State {
		uint32_t _numPacks = 1u;
		core::DynamicArray<Region> _sizes;
		core::DynamicArray<VoxLayer> _layers;
		core::DynamicArray<VoxModel> _xyzi;
		core::DynamicArray<VoxCamera> _cameras;
		core::DynamicArray<VoxROBJ> _robjs;

		bool _foundSceneGraph = false;
		VoxNodeId _rootNode = 0u;
		core::Map<VoxNodeId, VoxSceneGraphNode> _sceneGraph;
		core::Map<VoxNodeId, VoxNSHPNode> _nshp;
		core::Map<VoxNodeId, VoxNTRNNode> _ntrn;
		core::Map<VoxNodeId, VoxNGRPNode> _ngrp;
		core::Map<VoxNodeId, VoxNodeId> _parentNodes;

		~State() {
			for (VoxModel &m : _xyzi) {
				delete m.volume;
			}
		}
	};

	bool saveAttributes(const VoxAttributes& attributes, io::SeekableWriteStream& stream) const;

	bool saveChunk_LAYR(State& state, io::SeekableWriteStream& stream, int32_t modelId, const core::String& name, bool visible);
	bool saveChunk_XYZI(State& state, io::SeekableWriteStream& stream, const voxel::RawVolume* volume, const voxel::Region& region);
	bool saveChunk_SIZE(State& state, io::SeekableWriteStream& stream, const voxel::Region& region);
	bool saveChunk_PACK(State& state, io::SeekableWriteStream& stream, const SceneGraph& sceneGraph);
	bool saveChunk_RGBA(State& state, io::SeekableWriteStream& stream);

	// scene graph saving stuff
	bool saveChunk_nGRP(State& state, io::SeekableWriteStream& stream, VoxNodeId nodeId, uint32_t childNodes);
	bool saveChunk_nSHP(State& state, io::SeekableWriteStream& stream, VoxNodeId nodeId, int32_t modelId);
	bool saveChunk_nTRN(State& state, io::SeekableWriteStream& stream, VoxNodeId nodeId, VoxNodeId childNodeId, const glm::ivec3& mins, int layerId);
	bool saveSceneGraph(State& state, io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, int modelCount);

	void initPalette();
	void reset();

	bool readChunkHeader(io::SeekableReadStream& stream, VoxChunkHeader& header) const;
	bool readAttributes(VoxAttributes& attributes, io::SeekableReadStream& stream) const;

	bool checkVersionAndMagic(io::SeekableReadStream& stream) const;
	bool checkMainChunk(io::SeekableReadStream& stream) const;

	// first iteration
	bool loadChunk_MATL(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadChunk_MATT(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadChunk_IMAP(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadChunk_NOTE(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadChunk_PACK(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadChunk_RGBA(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadChunk_rOBJ(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadChunk_rCAM(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadChunk_SIZE(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadFirstChunks(State& state, io::SeekableReadStream& stream);

	// second iteration
	bool loadChunk_LAYR(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadChunk_XYZI(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadSecondChunks(State& state, io::SeekableReadStream& stream);

	// scene graph
	bool fillSceneGraph_r(State& state, uint32_t nodeId, voxel::SceneGraph& sceneGraph, int parentId);
	bool parseSceneGraphTranslation(VoxNTRNNode& transform, const VoxAttributes& attributes) const;
	bool parseSceneGraphRotation(VoxNTRNNode& transform, const VoxAttributes& attributes) const;
	bool loadChunk_nGRP(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadChunk_nSHP(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadChunk_nTRN(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header);
	bool loadSceneGraph(State& state, io::SeekableReadStream& stream);
	VoxNTRNNode traverseTransform(State& state, int32_t modelId) const;
	bool concatTransform(State& state, VoxNTRNNode& transform, VoxNodeId nodeId) const;
	glm::ivec3 calcTransform(State& state, const VoxNTRNNode& t, int x, int y, int z, const glm::vec3& pivot) const;
	glm::ivec3 calcTransform(State& state, const VoxNTRNNode& t, const glm::ivec3& pos, const glm::vec3& pivot) const;

public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, core::Array<uint32_t, 256> &palette) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
