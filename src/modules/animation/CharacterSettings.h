/**
 * @file
 */

#pragma once

#include "SkeletonAttribute.h"
#include <string>
#include <stdint.h>

namespace animation {

/**
 * @brief Attributes for the character meshes
 * @sa SkeletonAttribute
 */
struct CharacterSettings {
	SkeletonAttribute skeletonAttr;
	std::string race = "human";
	std::string gender = "male";
	std::string chest = "chests/blacksmith";
	std::string belt = "belts/blacksmith";
	std::string pants = "pants/blacksmith";
	std::string hand = "hands/blacksmith";
	std::string foot = "feets/blacksmith";
	std::string head = "heads/blacksmith";
	std::string shoulder = "shoulders/blacksmith";

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
};

extern bool loadCharacterSettings(const std::string& luaString, CharacterSettings& settings);

}
