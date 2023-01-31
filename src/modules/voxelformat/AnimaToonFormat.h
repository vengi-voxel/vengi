/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/DynamicArray.h"

namespace voxelformat {

/**
 * @brief Animatoon format
 *
 * @ingroup Formats
 */
class AnimaToonFormat : public RGBAFormat {
protected:
#if 0
	struct AnimaToonPosition {
		bool isModified;
		bool isLeftHandClosed;
		bool isRightHandClosed;
		core::DynamicArray<glm::vec3> meshPositions;
		core::DynamicArray<glm::quat> meshRotations;
		core::DynamicArray<glm::vec3> ikPositions;
		core::DynamicArray<glm::quat> ikRotations;
		core::DynamicArray<bool> ikModified;
	};
#endif

	enum AnimaToonVoxelState : uint8_t { inactive, active, hidden };

	struct AnimaToonVoxel {
		AnimaToonVoxelState state;
		uint8_t val;
		uint32_t rgba;
	};

	struct AnimaToonVolume {
		int xSize = 32;
		int ySize = 32;
		int zSize = 32;
		core::DynamicArray<AnimaToonVoxel> voxels;
	};

	bool loadGroupsRGBA(const core::String& filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, const voxel::Palette &palette) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, ThumbnailCreator thumbnailCreator) override {
		return false;
	}
};

}
