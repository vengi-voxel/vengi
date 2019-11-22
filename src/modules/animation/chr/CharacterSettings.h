/**
 * @file
 */

#pragma once

#include "CharacterSkeletonAttribute.h"
#include "CharacterMeshType.h"
#include "core/NonCopyable.h"
#include "core/String.h"
#include "core/Common.h"
#include <string>
#include <array>
#include <stdint.h>

namespace animation {

/**
 * @brief Attributes for the character meshes
 * @sa SkeletonAttribute
 */
struct CharacterSettings : public core::NonCopyable {
	CharacterSkeletonAttribute skeletonAttr;
	std::string race;
	std::string gender;
	std::string chest;
	std::string belt;
	std::string pants;
	std::string hand;
	std::string foot;
	std::string head;
	std::string shoulder;

	std::array<const std::string*, std::enum_value(CharacterMeshType::Max)> paths;
	char basePath[64] {};

	CharacterSettings();

	std::string fullPath(CharacterMeshType type, const char* name) const;
	std::string fullPath(CharacterMeshType type) const;

	/**
	 * @brief Get the original path the settings were loaded with
	 */
	const char* path(CharacterMeshType type) const;
	/**
	 * @brief Get the default path for the mesh type, but with a new name
	 */
	std::string path(CharacterMeshType type, const char *name) const;

	void copyFrom(const CharacterSettings& other);

	void setRace(const char *str);
	void setGender(const char *str);
	void setChest(const char *str);
	void setBelt(const char *str);
	void setPants(const char *str);
	void setHand(const char *str);
	void setFoot(const char *str);
	void setHead(const char *str);
	void setShoulder(const char *str);

	bool update();
};

extern std::string luaFilename(const char *character);
extern bool loadCharacterSettings(const std::string& luaString, CharacterSettings& settings);

}
