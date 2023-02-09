/**
 * @file
 */

#pragma once

#include "ShadowmapData.h"
#include "core/NonCopyable.h"
#include "core/Optional.h"
#include "core/collection/ConcurrentPriorityQueue.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Concurrency.h"
#include "core/concurrent/ThreadPool.h"
#include "render/BloomRenderer.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "video/Buffer.h"
#include "video/FrameBuffer.h"
#include "VoxelShader.h"
#include "ShadowmapShader.h"
#include "VoxelShaderConstants.h"
#include "voxel/Mesh.h"
#include "voxelrender/Shadow.h"
#include "core/GLM.h"
#include "core/Var.h"
#include "core/collection/Array.h"
#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace video {
class Camera;
}

namespace voxelformat {
class SceneGraphNode;
}

/**
 * Basic voxel rendering
 */
namespace voxelrender {

struct RenderContext : public core::NonCopyable {
	video::FrameBuffer frameBuffer;
	render::BloomRenderer bloomRenderer;
	bool sceneMode = false;

	bool init(const glm::ivec2 &size);
	void shutdown();
	bool resize(const glm::ivec2 &size);
};

/**
 * @brief Handles the shaders, vertex buffers and rendering of a voxel::RawVolume - including the mesh extraction part
 * @sa MeshRenderer
 * @sa voxel::RawVolume
 */
class RawVolumeRenderer : public core::NonCopyable {
public:
	static constexpr int MAX_VOLUMES = 2048;
protected:
	enum MeshType {
		MeshType_Opaque,
		MeshType_Transparency,
		MeshType_Max
	};
	struct State {
		bool _hidden = false;
		bool _gray = false;
		int32_t _vertexBufferIndex[MeshType_Max] {-1, -1};
		int32_t _indexBufferIndex[MeshType_Max] {-1, -1};
		glm::mat4 _model;
		glm::vec3 _pivot;
		video::Buffer _vertexBuffer[MeshType_Max];
		voxel::RawVolume* _rawVolume = nullptr;
		core::Optional<voxel::Palette> _palette;

		uint32_t indices(MeshType type) const {
			return _vertexBuffer[type].elements(_indexBufferIndex[type], 1, sizeof(voxel::IndexType));
		}

		bool hasData() const {
			return indices(MeshType_Opaque) > 0 || indices(MeshType_Transparency) > 0;
		}
	};
	core::Array<State, MAX_VOLUMES> _state {};
	typedef core::Array<voxel::Mesh*, MAX_VOLUMES> Meshes;
	typedef std::unordered_map<glm::ivec3, Meshes> MeshesMap;
	MeshesMap _meshes[MeshType_Max];

	uint64_t _paletteHash = 0;

	shader::VoxelData _voxelData;

	shader::VoxelData::FragData _voxelShaderFragData;
	shader::VoxelData::VertData _voxelShaderVertData;

	shader::VoxelShader& _voxelShader;
	shader::ShadowmapData _shadowMapUniformBlock;
	shader::ShadowmapShader& _shadowMapShader;
	voxelrender::Shadow _shadow;

	core::VarPtr _meshSize;
	core::VarPtr _shadowMap;
	core::VarPtr _bloom;

	struct ExtractRegion {
		ExtractRegion(const voxel::Region &_region, int _idx) : region(_region), idx(_idx) {
		}
		voxel::Region region;
		int idx;
	};
	using RegionQueue = core::DynamicArray<ExtractRegion>;
	RegionQueue _extractRegions;

	struct ExtractionCtx {
		ExtractionCtx() {}
		ExtractionCtx(const glm::ivec3& _mins, int _idx, voxel::ChunkMesh&& _mesh) :
				mins(_mins), idx(_idx), mesh(_mesh) {
		}
		glm::ivec3 mins {};
		int idx = -1;
		voxel::ChunkMesh mesh;

		inline bool operator<(const ExtractionCtx &rhs) const {
			return idx < rhs.idx;
		}
	};
	core::ThreadPool _threadPool { core::halfcpus(), "VolumeRndr" };
	core::AtomicInt _runningExtractorTasks { 0 };
	core::ConcurrentPriorityQueue<ExtractionCtx> _pendingQueue;
	voxel::Region calculateExtractRegion(int x, int y, int z, const glm::ivec3& meshSize) const;
	void updatePalette(int idx);
	bool updateBufferForVolume(int idx, MeshType type);

public:
	RawVolumeRenderer();

	void render(RenderContext &renderContext, const video::Camera& camera, bool shadow = true);
	void hide(int idx, bool hide);
	bool hidden(int idx) const;
	void gray(int idx, bool gray);
	bool grayed(int idx) const;

	int pendingExtractions() const;
	void clearPendingExtractions();
	void waitForPendingExtractions();

	template<class VISITOR>
	void visitPendingExtractions(VISITOR && func) const {
		for (const auto& e : _extractRegions) {
			func(e.idx, e.region);
		}
	}

	/**
	 * @brief Updates the vertex buffers manually
	 * @sa extract()
	 */
	bool updateBufferForVolume(int idx) {
		bool success = true;
		if (!updateBufferForVolume(idx, MeshType_Opaque)) {
			success = false;
		}
		if (!updateBufferForVolume(idx, MeshType_Transparency)) {
			success = false;
		}
		return success;
	}

	bool extractRegion(int idx, const voxel::Region& region);

	/**
	 * @param[in,out] volume The RawVolume pointer
	 * @return The old volume that was managed by the class, @c nullptr if there was none
	 *
	 * @sa volume()
	 */
	voxel::RawVolume* setVolume(int idx, voxel::RawVolume* volume, voxel::Palette* palette, bool deleteMesh = true);
	voxel::RawVolume* setVolume(int idx, voxelformat::SceneGraphNode& node, bool deleteMesh = true);
	bool setModelMatrix(int idx, const glm::mat4& model, const glm::vec3 &pivot, bool reset = true);

	bool empty(int idx = 0) const;
	/**
	 * @sa setVolume()
	 */
	voxel::RawVolume* volume(int idx = 0);
	const voxel::RawVolume* volume(int idx = 0) const;
	/**
	 * @note If you need the region of a particular volume, ask the volume
	 * @return The complete region of all managed volumes
	 */
	voxel::Region region() const;

	void setAmbientColor(const glm::vec3& color);
	void setDiffuseColor(const glm::vec3& color);
	void setSunPosition(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);

	void construct();

	/**
	 * @sa shutdown()
	 */
	bool init();

	bool scheduleExtractions(size_t maxExtraction = 1);
	void update();

	/**
	 * @return the managed voxel::RawVolume instance pointer, or @c nullptr if there is none set.
	 * @note You take the ownership of the returned volume pointers. Don't forget to delete them.
	 *
	 * @sa init()
	 */
	core::DynamicArray<voxel::RawVolume*> shutdown();
};

inline int RawVolumeRenderer::pendingExtractions() const {
	return (int)_extractRegions.size();
}

inline voxel::RawVolume* RawVolumeRenderer::volume(int idx) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	return _state[idx]._rawVolume;
}

inline const voxel::RawVolume* RawVolumeRenderer::volume(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	return _state[idx]._rawVolume;
}

}
