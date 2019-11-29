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

extern std::string luaFilename(const char *character);

class AnimationSettings {
private:
	std::vector<std::string> _types;

public:
	static constexpr const size_t MAX_ENTRIES {64};

	AnimationSettings(const std::vector<std::string> &types);
	virtual ~AnimationSettings();

	std::string paths[MAX_ENTRIES];
	BoneIds boneIdsArray[MAX_ENTRIES];
	std::string basePath;

	const std::string& type(size_t idx) const;
	int getIdxForName(const char *name) const;
	std::string fullPath(const char* type, const char* name) const;
	std::string fullPath(const char* type) const;
	std::string fullPath(int idx) const;

	/**
	 * @brief Get the original path the settings were loaded with
	 */
	const std::string& path(const char* type) const;
	/**
	 * @brief Get the default path for the mesh type, but with a new name
	 */
	std::string path(const char* type, const char *name) const;
	bool setPath(const char* type, const char *str);

	BoneIds* boneIds(const char *name);
	const BoneIds& boneIds(size_t id) const;
};

inline const BoneIds& AnimationSettings::boneIds(size_t id) const {
	return boneIdsArray[id];
}

}
