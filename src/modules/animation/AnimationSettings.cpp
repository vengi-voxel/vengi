/**
 * @file
 */

#include "AnimationSettings.h"
#include "core/String.h"

namespace animation {

std::string luaFilename(const char *character) {
	return core::string::format("%s.lua", character);
}

AnimationSettings::AnimationSettings(const std::vector<std::string> &types) :
		_types(types) {
}

AnimationSettings::~AnimationSettings() {
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

std::string AnimationSettings::fullPath(const char *type,
		const char *name) const {
	const std::string &p = path(type, name);
	return core::string::format("%s/%s.vox", basePath.c_str(), p.c_str());
}

std::string AnimationSettings::fullPath(const char *type) const {
	return fullPath(getIdxForName(type));
}

std::string AnimationSettings::fullPath(int idx) const {
	if (idx < 0 || idx >= (int) MAX_ENTRIES) {
		static const std::string EMPTY;
		return EMPTY;
	}
	return core::string::format("%s/%s.vox", basePath.c_str(),
			paths[idx].c_str());
}

const std::string& AnimationSettings::path(const char *type) const {
	const int idx = getIdxForName(type);
	if (idx < 0 || idx >= (int) MAX_ENTRIES) {
		static const std::string EMPTY;
		return EMPTY;
	}
	return paths[idx];
}

std::string AnimationSettings::path(const char *type, const char *name) const {
	const std::string &p = path(type);
	return core::string::format("%s/%s", p.c_str(), name);
}

bool AnimationSettings::setPath(const char *type, const char *str) {
	const int idx = getIdxForName(type);
	if (idx < 0 || idx >= (int) MAX_ENTRIES) {
		return false;
	}
	paths[idx] = str;
	return true;
}

BoneIds* AnimationSettings::boneIds(const char *name) {
	const int idx = getIdxForName(name);
	if (idx < 0 || idx >= (int) MAX_ENTRIES) {
		return nullptr;
	}
	return &boneIdsArray[idx];
}

}
