/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Optional.h"
#include "core/SharedPtr.h"
#include "core/Var.h"
#include "core/collection/Array.h"
#include "core/collection/DynamicMap.h"
#include "core/collection/PriorityQueue.h"
#include "core/collection/Queue.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"
#include "video/Types.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Mesh.h"

#include "core/GLM.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include <glm/mat4x4.hpp>

namespace voxel {

static constexpr int MAX_VOLUMES = 2048;

enum MeshType { MeshType_Opaque, MeshType_Transparency, MeshType_Max };

/**
 * @brief Handles the mesh extraction of the volumes
 *
 * @note This class doesn't own the @c voxel::RawVolume instances. It's up to the caller to inform this class about
 * deleted or added volumes.
 */
class MeshState {
public:
	typedef core::Array<voxel::Mesh *, MAX_VOLUMES> Meshes;
	typedef core::DynamicMap<glm::ivec3, Meshes, 531, glm::hash<glm::ivec3>> MeshesMap;

private:
	struct VolumeData {
		voxel::RawVolume *_rawVolume = nullptr;
		core::Optional<palette::Palette> _palette;
		core::Optional<palette::NormalPalette> _normalPalette;
		bool _hidden = false;
		bool _gray = false;
		// if all axes scale positive: cull the back face
		// if one or three axes are negative, then cull the front face
		video::Face _cullFace = video::Face::Back;
		int _reference = -1;
		glm::mat4 _model{1.0f};
		glm::vec3 _mins{0.0f};
		glm::vec3 _maxs{0.0f};
		/**
		 * @brief Applies the model matrix
		 * @note Used for sorting (for transparency)
		 */
		glm::vec3 centerPos(bool applyModel = true) const;
	};
	typedef core::Array<VolumeData, MAX_VOLUMES> Volumes;

	struct ExtractionResult {
		ExtractionResult() {
		}
		ExtractionResult(const glm::ivec3 &_mins, int _idx, voxel::ChunkMesh &&_mesh)
			: mins(_mins), idx(_idx), mesh(core::move(_mesh)) {
		}
		glm::ivec3 mins{};
		int idx = -1;
		voxel::ChunkMesh mesh{0, 0, true};

		inline bool operator<(const ExtractionResult &rhs) const {
			return idx < rhs.idx;
		}
	};

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

	voxel::Region calculateExtractRegion(int x, int y, int z, const glm::ivec3 &meshSize) const;
	core::Queue<int> _pendingMeshes;
	core::VarPtr _meshMode;
	bool deleteMeshes(const glm::ivec3 &pos, int idx);
	bool runScheduledExtractions(size_t maxExtraction = 0);
	bool deleteMeshes(int idx);
	void addOrReplaceMeshes(MeshState::ExtractionResult &result, MeshType type);

public:
	MeshState();
	void clearMeshes();
	const MeshesMap &meshes(MeshType type) const;
	/**
	 * @brief This will transfer the extracted meshes into the mesh state and make
	 * it available to others
	 */
	int pop();
	void count(MeshType meshType, int idx, size_t &vertCount, size_t &normalsCount, size_t &indCount) const;
	const palette::Palette &palette(int idx) const;
	const palette::NormalPalette &normalsPalette(int idx) const;

	bool hasNormals() const {
		return meshMode() == voxel::SurfaceExtractionType::MarchingCubes;
	}

	/**
	 * @brief Extracts all the pending regions
	 * @note This method is blocking
	 * @sa scheduleRegionExtraction()
	 */
	void extractAllPending();
	/**
	 * @return the amount of pending extractions
	 */
	int pendingExtractions() const;
	void clearPendingExtractions();
	int pendingMeshes() const;

	/**
	 * @sa shutdown()
	 */
	bool init();
	void construct();

	const glm::vec3 &mins(int idx) const;
	const glm::vec3 &maxs(int idx) const;
	/**
	 * @brief Applies the model matrix
	 * @note Used for sorting (for transparency)
	 */
	glm::vec3 centerPos(int idx, bool applyModel) const;
	const glm::mat4 &model(int idx) const;
	bool setModelMatrix(int idx, const glm::mat4 &model, const glm::vec3 &mins, const glm::vec3 &maxs);

	/**
	 * @return @c true if the mesh mode was changed and the consumer should be aware that all meshes should get cleaned
	 * up
	 * @sa marchingCubes()
	 */
	bool update();
	/**
	 * @sa update()
	 */
	voxel::SurfaceExtractionType meshMode() const;

	/**
	 * @brief Split the region according to the configured mesh size
	 * @note Without calling @c extractAllPending() or @c update() the mesh won't get extracted
	 * @return @c true if the mesh should get deleted in the renderer
	 */
	bool scheduleRegionExtraction(int idx, const voxel::Region &region);

	bool sameNormalPalette(int idx, const palette::NormalPalette *palette) const;

	[[nodiscard]] voxel::RawVolume *setVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette,
											  palette::NormalPalette *normalPalette, bool meshDelete,
											  bool &meshDeleted);

	/**
	 * @return the managed voxel::RawVolume instance pointer, or @c nullptr if there is none set.
	 * @note You take the ownership of the returned volume pointers. Don't forget to delete them.
	 *
	 * @sa init()
	 */
	[[nodiscard]] core::Buffer<voxel::RawVolume *> shutdown();

	[[nodiscard]] voxel::RawVolume *volume(int idx);
	[[nodiscard]] const voxel::RawVolume *volume(int idx) const;

	void hide(int idx, bool hide);
	bool hidden(int idx) const;
	void gray(int idx, bool gray);
	bool grayed(int idx) const;

	// for scaling on 1 or 3 axes negative we need to flip the face culling
	video::Face cullFace(int idx) const;
	void setCullFace(int idx, video::Face face);

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

inline int MeshState::pendingMeshes() const {
	return (int)_pendingMeshes.size();
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

} // namespace voxel
