/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <stdint.h>

namespace voxel {
class RawVolume;
}

namespace voxedit {

enum class SceneJobType : uint8_t {
	None,
	CropVolume,
	ScaleUpVolume,
	ScaleDownVolume,
	ResizeVolume,
	FillVolume,
	ClearVolume,
	DeleteSelectedVolume,
	HollowVolume,
	FillHollowVolume,
	SplitObjects,
	ColorToModel,
	SplatMerge,
	Max
};

const char *sceneJobText(SceneJobType type);

/**
 * @brief Snapshot data for a potential splat merge target.
 *
 * The worker owns copied target volumes and can produce modified target results
 * without touching the live scene graph.
 */
struct SceneJobSplatTarget {
	int nodeId = InvalidNodeId;
	voxel::RawVolume *volume = nullptr;
	palette::Palette palette;
	voxel::Region worldRegion = voxel::Region::InvalidRegion;

	~SceneJobSplatTarget();
	SceneJobSplatTarget() = default;
	SceneJobSplatTarget(const SceneJobSplatTarget &other);
	SceneJobSplatTarget &operator=(const SceneJobSplatTarget &other);
	SceneJobSplatTarget(SceneJobSplatTarget &&other) noexcept;
	SceneJobSplatTarget &operator=(SceneJobSplatTarget &&other) noexcept;
};

/**
 * @brief Description of a pending scene job that can be started later.
 *
 * Requests are intentionally small and only capture user intent and immutable
 * command arguments. Scene snapshots are created when the request becomes the
 * active job so queued commands operate on the result of previous jobs.
 */
struct SceneJobRequest {
	SceneJobType type = SceneJobType::None;
	int nodeId = InvalidNodeId;
	core::String text;
	voxel::Region region = voxel::Region::InvalidRegion;
	glm::ivec3 size{0};
	voxel::Voxel voxel;
	bool overrideVoxels = false;
	core::Buffer<uint8_t> paletteIndices;
};

/**
 * @brief Owned volume replacement produced by a scene job.
 *
 * The result keeps ownership of the volume until the main thread applies it to
 * the scene graph. It is used for both direct replacements and multi-node jobs
 * like splat merge.
 */
struct SceneJobVolumeResult {
	int nodeId = InvalidNodeId;
	voxel::RawVolume *volume = nullptr;
	voxel::Region modifiedRegion = voxel::Region::InvalidRegion;

	SceneJobVolumeResult() = default;
	~SceneJobVolumeResult();
	SceneJobVolumeResult(const SceneJobVolumeResult &other);
	SceneJobVolumeResult &operator=(const SceneJobVolumeResult &other);
	SceneJobVolumeResult(SceneJobVolumeResult &&other) noexcept;
	SceneJobVolumeResult &operator=(SceneJobVolumeResult &&other) noexcept;

	voxel::RawVolume *releaseVolume();
};

/**
 * @brief Model node data produced by structural background jobs.
 *
 * Worker threads can create detached volumes, while the main thread later wraps
 * them in scene graph nodes and attaches them safely.
 */
struct SceneJobNewNode {
	int sourceNodeId = InvalidNodeId;
	int parentNodeId = InvalidNodeId;
	core::String name;
	palette::Palette palette;
	voxel::RawVolume *volume = nullptr;

	SceneJobNewNode() = default;
	~SceneJobNewNode();
	SceneJobNewNode(const SceneJobNewNode &other);
	SceneJobNewNode &operator=(const SceneJobNewNode &other);
	SceneJobNewNode(SceneJobNewNode &&other) noexcept;
	SceneJobNewNode &operator=(SceneJobNewNode &&other) noexcept;

	voxel::RawVolume *releaseVolume();
};

/**
 * @brief Result object returned by a background scene job.
 *
 * The worker thread only fills this structure with owned, detached data. The
 * main thread consumes it in @c SceneManager::updateSceneJob() and performs all
 * scene graph, memento and renderer updates.
 */
struct SceneJobResult {
	SceneJobType type = SceneJobType::None;
	int nodeId = InvalidNodeId;
	voxel::RawVolume *volume = nullptr;
	voxel::Region modifiedRegion = voxel::Region::InvalidRegion;
	core::DynamicArray<SceneJobVolumeResult> volumes;
	core::DynamicArray<SceneJobNewNode> newNodes;
	bool removeSourceNode = false;
	bool success = false;
	core::String error;

	SceneJobResult() = default;
	~SceneJobResult();
	SceneJobResult(const SceneJobResult &) = delete;
	SceneJobResult &operator=(const SceneJobResult &) = delete;
	SceneJobResult(SceneJobResult &&other) noexcept;
	SceneJobResult &operator=(SceneJobResult &&other) noexcept;

	voxel::RawVolume *releaseVolume();
};

voxel::Region sceneJobModifiedRegionForResize(const voxel::Region &oldRegion, const voxel::Region &newRegion);
SceneJobResult makeVolumeOperationSceneJobResult(SceneJobType type, int nodeId, voxel::RawVolume *snapshot,
												 const voxel::Region &selectionRegion, const voxel::Voxel &voxel,
												 bool overrideVoxels);

} // namespace voxedit
