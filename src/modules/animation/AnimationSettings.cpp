/**
 * @file
 */

#include "AnimationSettings.h"
#include "core/String.h"
#include "core/Assert.h"

namespace animation {

std::string luaFilename(const char *character) {
	return core::string::format("%s.lua", character);
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
	return core::string::format("%s/%s/%s", _types[idx].c_str(), paths[idx].c_str(), name);
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
