/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include <glm/vec3.hpp>
#include "voxelformat/Format.h"
#include <glm/gtc/quaternion.hpp>

namespace voxelformat {

/**
 * @brief Animatoon .scn format
 *
 * JSON document with gzip+base64 voxel volumes in @c ModelSave. Scene hierarchy
 * (node names, parents, volume size) is hard-coded per @c SceneName because
 * Animatoon 3.0 embeds that in the Unity scene, not the file.
 *
 * @c savedPositionsList stores one Unity @c SavePositions JSON string per
 * timeline frame. @c meshPositions / @c meshRotations are local TRS of
 * @c FrameSaver.usedMeshes (already IK-baked). @c ModelSave parts share one
 * rest-pose volume; joints are inferred from occupancy (prefab Pivot is not
 * stored). Voxels are remapped with @c math::CoordinateSystem::Unity. Unity
 * local TRS is converted with @c scenegraph::convertUnityToVengi and scaled
 * by 10 (PicaVoxel VoxelSize 0.1).
 *
 * @todo Animations are not yet working
 * @ingroup Formats
 */
class AnimaToonFormat : public RGBAFormat {
protected:
	struct AnimaToonPosition {
		bool isModified;
		bool isLeftHandClosed;
		bool isRightHandClosed;
		core::Buffer<glm::vec3> meshPositions;
		core::Buffer<glm::quat> meshRotations;
		core::Buffer<glm::vec3> ikPositions;
		core::Buffer<glm::quat> ikRotations;
		core::Buffer<bool> ikModified;
	};

	enum AnimaToonVoxelState : uint8_t { inactive, active, hidden };

	struct AnimaToonVoxel {
		AnimaToonVoxelState state;
		uint8_t val;
		uint32_t rgba;
	};

	struct AnimaToonVolume {
		int xSize = 40;
		int ySize = 40;
		int zSize = 40;
		core::Buffer<AnimaToonVoxel> voxels;
	};

	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override {
		return false;
	}

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"AnimaToon", "", {"scn"}, {}, VOX_FORMAT_FLAG_ANIMATION};
		return f;
	}
};

} // namespace voxelformat
