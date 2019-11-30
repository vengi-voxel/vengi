/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "Animation.h"
#include "BoneId.h"
#include <array>
#include <string>
#include <vector>

namespace animation {

struct SkeletonAttributeMeta;

extern std::string luaFilename(const char *character);

class AnimationSettings {
private:
	std::vector<std::string> _types;

public:
	static constexpr const size_t MAX_ENTRIES {64};

	std::string paths[MAX_ENTRIES];
	BoneIds boneIdsArray[MAX_ENTRIES];
	std::string basePath;

	const std::vector<std::string>& types() const;
	const std::string& type(size_t idx) const;
	int getIdxForName(const char *name) const;
	/**
	 * @brief Configure the available mesh types.
	 *
	 * @note They must match the bone configuration. See the lua script for mappings.
	 */
	void setTypes(const std::vector<std::string>& types);

	/**
	 * @brief Assemble the full path to the model that should be used for the given mesh type index.
	 */
	std::string fullPath(int idx, const char *name = nullptr) const;

	/**
	 * @brief Get the default path for the mesh type, but with a new name
	 */
	std::string path(int idx, const char *name = nullptr) const;
	bool setPath(int idx, const char *str);

	const BoneIds& boneIds(int id) const;
	BoneIds& boneIds(int id);
};

inline const std::vector<std::string>& AnimationSettings::types() const {
	return _types;
}

/**
 * @brief Load the given lua string and fill the AnimationSettings values as well as the skeleton attributes identified by the via base pointer and the meta data iterator
 * @param[in] luaString
 * @param[out] settings the AnimationSettings to fill
 * @param[out] skeletonAttr The base pointer to the skeleton attributes
 * @param[in] metaIter a null terminated array of name and offsets for the individual skeleton attributes
 * @return @c true on success, @c false on failure
 */
extern bool loadAnimationSettings(const std::string& luaString, AnimationSettings& settings, void* skeletonAttr, const SkeletonAttributeMeta* metaIter);

}
