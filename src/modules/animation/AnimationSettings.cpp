/**
 * @file
 */

#include "AnimationSettings.h"
#include "core/String.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/Common.h"
#include "LUAShared.h"
#include "SkeletonAttribute.h"

namespace animation {

std::string luaFilename(const char *character) {
	return core::string::format("%s.lua", character);
}

bool loadAnimationSettings(const std::string& luaString, AnimationSettings& settings, void* skeletonAttr, const animation::SkeletonAttributeMeta* metaIter) {
	if (luaString.empty()) {
		Log::warn("empty animation settings can't get loaded");
		return false;
	}

	lua::LUA lua;
	lua.reg("settings", settingsFuncs);
	lua.reg("bone", boneFuncs);
	luaanim_boneidsregister(lua.state());

	if (!lua.load(luaString)) {
		Log::error("%s", lua.error().c_str());
		return false;
	}

	lua.newGlobalData<AnimationSettings>("Settings", &settings);
	if (!lua.execute("init", LUA_MULTRET)) {
		Log::error("%s", lua.error().c_str());
		return false;
	}

	// TODO: sanity checks for the amount of values on the stack and the set values in the attributes

	for (; metaIter->name; ++metaIter) {
		const animation::SkeletonAttributeMeta& meta = *metaIter;
		float *saVal = (float*)(((char*)&skeletonAttr) + meta.offset);
		if (lua.valueFloatFromTable(meta.name, saVal)) {
			Log::debug("Skeleton attribute value for %s: %f", meta.name, *saVal);
		} else {
			Log::debug("Skeleton attribute value for %s not given - use default: %f", meta.name, *saVal);
		}
	}

	return true;
}


void AnimationSettings::setTypes(const std::vector<std::string>& types) {
	_types = types;
}

const std::string& AnimationSettings::type(size_t idx) const {
	if (idx >= (int) MAX_ENTRIES) {
		static const std::string EMPTY;
		return EMPTY;
	}
	return _types[idx];
}

int AnimationSettings::getIdxForName(const char *name) const {
	for (size_t i = 0; i < _types.size(); ++i) {
		if (_types[i] == name) {
			return i;
		}
	}
	return -1;
}

std::string AnimationSettings::fullPath(int idx, const char *name) const {
	if (idx < 0 || idx >= (int) MAX_ENTRIES) {
		static const std::string EMPTY;
		return EMPTY;
	}
	if (name == nullptr) {
		name = paths[idx].c_str();
	}
	return core::string::format("%s/%s/%s.vox", basePath.c_str(), _types[idx].c_str(), name);
}

std::string AnimationSettings::path(int idx, const char *name) const {
	if (idx < 0 || idx >= (int) MAX_ENTRIES) {
		static const std::string EMPTY;
		return EMPTY;
	}
	if (name == nullptr) {
		name = paths[idx].c_str();
	}
	return core::string::format("%s/%s", _types[idx].c_str(), name);
}

bool AnimationSettings::setPath(int idx, const char *str) {
	core_assert(idx >= 0 && idx < (int) MAX_ENTRIES);
	paths[idx] = str;
	return true;
}

const BoneIds& AnimationSettings::boneIds(int idx) const {
	core_assert(idx >= 0 && idx < (int) MAX_ENTRIES);
	return boneIdsArray[idx];
}

BoneIds& AnimationSettings::boneIds(int idx) {
	core_assert(idx >= 0 && idx < (int) MAX_ENTRIES);
	return boneIdsArray[idx];
}

}
