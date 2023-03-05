/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/Tokenizer.h"
#include "core/collection/DynamicArray.h"

namespace voxelformat {

/**
 * @brief Animatoon format
 *
 * @ingroup Formats
 *
 * @todo Not yet working
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
		int xSize = 32;
		int ySize = 32;
		int zSize = 32;
		core::DynamicArray<AnimaToonVoxel> voxels;
	};

	template<class FUNC>
	bool parseJsonArray(core::Tokenizer &tokenizer, FUNC func) {
		if (!tokenizer.hasNext()) {
			return false;
		}
		if (tokenizer.next() != "[") {
			return false;
		}
		while (tokenizer.hasNext()) {
			const core::String &token = tokenizer.next();
			if (token == "]") {
				return true;
			}
			func(token);
		}
		return false;
	}

	bool loadGroupsRGBA(const core::String& filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, const voxel::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) override {
		return false;
	}
};

}
