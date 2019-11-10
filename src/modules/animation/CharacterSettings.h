/**
 * @file
 */

#pragma once

#include "SkeletonAttribute.h"
#include "CharacterMeshType.h"
#include "core/NonCopyable.h"
#include "core/String.h"
#include <string>
#include <stdint.h>

namespace animation {

/**
 * @brief Attributes for the character meshes
 * @sa SkeletonAttribute
 */
struct CharacterSettings : public core::NonCopyable {
	SkeletonAttribute skeletonAttr;
	std::string race;
	std::string gender;
	std::string chest;
	std::string belt;
	std::string pants;
	std::string hand;
	std::string foot;
	std::string head;
	std::string shoulder;

	std::array<const std::string*, std::enum_value(CharacterMeshType::Max)> paths {};
	char basePath[64] {};

	CharacterSettings() {}

	inline std::string fullPath(CharacterMeshType type) const {
		return core::string::format("%s/%s.vox", basePath, path(type));
	}

	const char* path(CharacterMeshType type) const {
		if (paths[std::enum_value(type)] == nullptr) {
			return "";
		}
		return paths[std::enum_value(type)]->c_str();
	}

	void copyFrom(const CharacterSettings& other) {
		skeletonAttr = other.skeletonAttr;
		race = other.race;
		gender = other.gender;
		chest = other.chest;
		belt = other.belt;
		pants = other.pants;
		hand = other.hand;
		foot = other.foot;
		head = other.head;
		shoulder = other.shoulder;
		paths = {};
		memcpy(basePath, other.basePath, sizeof(basePath));
		update();
	}

	void setRace(const char *str) {
		race = str;
	}

	void setGender(const char *str) {
		gender = str;
	}

	void setChest(const char *str) {
		chest = str;
	}

	void setBelt(const char *str) {
		belt = str;
	}

	void setPants(const char *str) {
		pants = str;
	}

	void setHand(const char *str) {
		hand = str;
	}

	void setFoot(const char *str) {
		foot = str;
	}

	void setHead(const char *str) {
		head = str;
	}

	void setShoulder(const char *str) {
		shoulder = str;
	}

	bool update();
};

extern std::string luaFilename(const char *character);
extern bool loadCharacterSettings(const std::string& luaString, CharacterSettings& settings);

}
