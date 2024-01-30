/**
 * @file
 */

#include "core/Var.h"
#include "core/Optional.h"
#include "core/SharedPtr.h"
#include "core/collection/Array.h"
#include "core/collection/ConcurrentPriorityQueue.h"
#include "core/collection/PriorityQueue.h"
#include "core/concurrent/ThreadPool.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Mesh.h"
#include "palette/Palette.h"
#include <unordered_map>

#include "core/GLM.h"
#include "voxel/RawVolume.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/hash.hpp>

namespace voxelrender {

static constexpr int MAX_VOLUMES = 2048;

enum MeshType { MeshType_Opaque, MeshType_Transparency, MeshType_Max };

// TODO: extract the mesh creation from RawVolumeRenderer and use it here
// this would allow us to perform the loading of the scene graph in a separate
// thread and the mesh extraction too, before we have to hand over to the
// renderer thread
class MeshState {
public:
	struct VolumeData {
		voxel::RawVolume *_rawVolume;
		core::Optional<palette::Palette> _palette;
		bool _hidden = false;
		bool _gray = false;
		int _reference = -1;
	};

	typedef core::Array<voxel::Mesh *, MAX_VOLUMES> Meshes;
	typedef core::Array<VolumeData, MAX_VOLUMES> Volumes;
	typedef std::unordered_map<glm::ivec3, Meshes> MeshesMap;

	struct ExtractionCtx {
		ExtractionCtx() {
		}
		ExtractionCtx(const glm::ivec3 &_mins, int _idx, voxel::ChunkMesh &&_mesh)
			: mins(_mins), idx(_idx), mesh(_mesh) {
		}
		glm::ivec3 mins{};
		int idx = -1;
		voxel::ChunkMesh mesh;

		inline bool operator<(const ExtractionCtx &rhs) const {
			return idx < rhs.idx;
		}
	};

private:
	MeshesMap _meshes[MeshType_Max];
	Volumes _volumeData;
	core::VarPtr _meshSize;

	struct ExtractRegion {
		ExtractRegion(const voxel::Region &_region, int _idx, bool _visible)
			: region(_region), idx(_idx), visible(_visible) {
		}
		ExtractRegion() {
		}
		voxel::Region region{};
		int idx = 0;
		bool visible = false;

		inline bool operator<(const ExtractRegion &rhs) const {
			return idx < rhs.idx && visible < rhs.visible;
		}
	};
	using RegionQueue = core::PriorityQueue<ExtractRegion>;
	RegionQueue _extractRegions;

	core::AtomicInt _runningExtractorTasks { 0 };
	core::AtomicInt _pendingExtractorTasks { 0 };
	voxel::Region calculateExtractRegion(int x, int y, int z, const glm::ivec3& meshSize) const;
	core::ThreadPool _threadPool { core::halfcpus(), "VolumeRndr" };
	core::ConcurrentPriorityQueue<MeshState::ExtractionCtx> _pendingQueue;
	core::VarPtr _meshMode;

public:
	const MeshesMap &meshes(MeshType type) const;
	int pop();
	bool deleteMeshes(const glm::ivec3 &pos, int idx);
	bool deleteMeshes(int idx);
	void clear();
	void count(MeshType meshType, int idx, size_t &vertCount, size_t &normalsCount, size_t &indCount) const;
	const palette::Palette &palette(int idx) const;

	bool scheduleExtractions(size_t maxExtraction = 1);
	int pendingExtractions() const;
	void clearPendingExtractions();
	void waitForPendingExtractions();
	bool init();
	void construct();
	bool update();
	bool marchingCubes() const;

	/**
	 * @return @c true if the mesh should get deleted in the renderer
	 */
	bool extractRegion(int idx, const voxel::Region& region);

	voxel::RawVolume *setVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette, bool meshDelete, bool &meshDeleted);

	core::DynamicArray<voxel::RawVolume *> shutdown();

	void setVolume(int idx, voxel::RawVolume *volume);
	void setPalette(int idx, palette::Palette *palette);

	voxel::RawVolume *volume(int idx);
	const voxel::RawVolume *volume(int idx) const;

	void hide(int idx, bool hide);
	bool hidden(int idx) const;
	void gray(int idx, bool gray);
	bool grayed(int idx) const;

	/**
	 * @brief In case of a reference - this gives us the index for the
	 * referenced object
	 */
	int resolveIdx(int idx) const;
	/**
	 * @brief Allows to render the same model with different transforms and
	 * palettes
	 */
	void setReference(int idx, int referencedIdx);
	void resetReferences();
	int reference(int idx) const;
};

using MeshStatePtr = core::SharedPtr<MeshState>;

inline int MeshState::pendingExtractions() const {
	return (int)_extractRegions.size();
}

inline voxel::RawVolume *MeshState::volume(int idx) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	return _volumeData[idx]._rawVolume;
}

inline const voxel::RawVolume *MeshState::volume(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	return _volumeData[idx]._rawVolume;
}

} // namespace voxelrender
