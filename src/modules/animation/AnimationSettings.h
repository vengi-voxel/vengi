/**
 * @file
 */

#pragma once

#include "core/ArrayLength.h"
#include "Animation.h"
#include "BoneId.h"
#include "core/collection/Array.h"
#include "core/String.h"
#include "core/Enum.h"
#include <vector>

namespace animation {

struct SkeletonAttributeMeta;
struct SkeletonAttribute;

extern core::String luaFilename(const char *character);

/**
 * @ingroup Animation
 */
class AnimationSettings {
public:
	enum class Type {
		Bird, Character, Max
	};
	static constexpr const char *TypeStrings[] = {
		"bird", "character"
	};
	static_assert((int)Type::Max == lengthof(TypeStrings));

private:
	std::vector<core::String> _meshTypes;
	int8_t _boneIndices[core::enumVal(BoneId::Max)];
	// current position in the bone index mapping array
	uint8_t _currentBoneIdx = 0u;

	Type _type = Type::Max;

public:
	static constexpr const size_t MAX_ENTRIES {64};

	core::String paths[MAX_ENTRIES];
	BoneIds boneIdsArray[MAX_ENTRIES];
	core::String basePath;

	void reset();

	/**
	 * @brief Call this after all settings have been set properly.
	 */
	bool init();

	/**
	 * @brief Register a boneId that is not directly attached to a mesh type, but used anyway.
	 * This is e.g. the case for items that are visible when a character carries them. But are
	 * otherwise not part of the model itself. But they still must be taken into account in the
	 * skeleton to put the item mesh to the correct location.
	 */
	bool registerBoneId(BoneId boneId);

	/**
	 * @return @c -1 if no mapping could get found
	 */
	int8_t mapBoneIdToArrayIndex(BoneId boneId) const;

	const std::vector<core::String>& types() const;
	const core::String& meshType(size_t meshTypeIdx) const;
	int getMeshTypeIdxForName(const char *name) const;
	/**
	 * @brief Configure the available mesh types.
	 *
	 * @note They must match the bone configuration. See the lua script for mappings.
	 */
	void setMeshTypes(const std::vector<core::String>& meshTypes);

	/**
	 * @brief Assemble the full path to the model that should be used for the given mesh type index.
	 */
	core::String fullPath(int meshTypeIdx, const char *name = nullptr) const;

	/**
	 * @brief Get the default path for the mesh type, but with a new name
	 */
	core::String path(int meshTypeIdx, const char *name = nullptr) const;
	bool setPath(int meshTypeIdx, const char *str);

	Type type() const;
	void setType(Type type);

	const BoneIds& boneIds(int meshTypeIdx) const;
	BoneIds& boneIds(int meshTypeIdx);
};

inline const std::vector<core::String>& AnimationSettings::types() const {
	return _meshTypes;
}

inline void AnimationSettings::setType(Type type) {
	_type = type;
}

inline AnimationSettings::Type AnimationSettings::type() const {
	return _type;
}

/**
 * @brief Load the given lua string and fill the AnimationSettings values as well as the skeleton attributes identified by the via base pointer and the meta data iterator
 * @param[out] settings the AnimationSettings to fill
 * @param[out] skeletonAttr The base pointer to the skeleton attributes
 * @return @c true on success, @c false on failure
 */
extern bool loadAnimationSettings(const core::String& luaString, AnimationSettings& settings, SkeletonAttribute* skeletonAttr);

}
