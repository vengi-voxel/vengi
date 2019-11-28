/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/Common.h"
#include "Animation.h"
#include "BoneId.h"
#include <array>

namespace animation {

static inline std::string luaFilename(const char *character) {
	return core::string::format("%s.lua", character);
}

template<typename T>
struct AnimationSettings {
	static constexpr const size_t MAX_ENTRIES {64};
	virtual ~AnimationSettings() {}

	std::string paths[MAX_ENTRIES];
	BoneIds boneIdsArray[MAX_ENTRIES];
	std::string basePath;

	std::string fullPath(T type, const char* name) const {
		const std::string& p = path(type, name);
		return core::string::format("%s/%s.vox", basePath.c_str(), p.c_str());
	}

	std::string fullPath(T type) const {
		const std::string& p = path(type);
		return core::string::format("%s/%s.vox", basePath.c_str(), p.c_str());
	}

	/**
	 * @brief Get the original path the settings were loaded with
	 */
	const std::string& path(T type) const {
		if (paths[std::enum_value(type)].empty()) {
			static const std::string EMPTY;
			return EMPTY;
		}
		return paths[std::enum_value(type)];
	}

	/**
	 * @brief Get the default path for the mesh type, but with a new name
	 */
	std::string path(T type, const char *name) const {
		return core::string::format("%s/%s", animation::toString(type), name);
	}

	inline void setPath(T type, const char *str) {
		paths[std::enum_value(type)] = str;
	}

	virtual T getMeshTypeIdForName(const char *name) = 0;

	inline BoneIds* boneIds(const char *name) {
		T id = getMeshTypeIdForName(name);
		if (id == T::Max) {
			return nullptr;
		}
		return &boneIdsArray[(int)id];
	}

	inline const BoneIds& boneIds(T id) const {
		core_assert(id != T::Max);
		return boneIdsArray[(int)id];
	}

	inline const BoneIds& boneIds(size_t id) const {
		return boneIdsArray[id];
	}
};

}
