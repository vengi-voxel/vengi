/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Animatoon format
 *
 * @ingroup Formats
 *
 * @todo Animations are not yet working
 */
class AnimaToonFormat : public RGBAFormat {
protected:
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
		core::DynamicArray<AnimaToonVoxel> voxels;
	};

	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override {
		return false;
	}

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"AnimaToon", {"scn"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_ANIMATION};
		return f;
	}
};

} // namespace voxelformat
