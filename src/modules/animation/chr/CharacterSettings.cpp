/**
 * @file
 */

#include "CharacterSettings.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Array.h"
#include "commonlua/LUA.h"
#include "LUAFunctions.h"

namespace animation {

std::string luaFilename(const char *character) {
	return core::string::format("%s.lua", character);
}

bool loadCharacterSettings(const std::string& luaString, CharacterSettings& settings) {
	if (luaString.empty()) {
		Log::warn("empty character settings can't get loaded");
		return false;
	}
	// also change the voxel editor lua script saving
	static const luaL_Reg funcs[] = {
		{ "setRace", luaChr_SetRace },
		{ "setGender", luaChr_SetGender },
		{ "setChest", luaChr_SetChest },
		{ "setBelt", luaChr_SetBelt },
		{ "setPants", luaChr_SetPants },
		{ "setHand", luaChr_SetHand },
		{ "setFoot", luaChr_SetFoot },
		{ "setHead", luaChr_SetHead },
		{ "setShoulder", luaChr_SetShoulder },

		{ "setScaler", luaChr_SetScaler },
		{ "setHeadScale", luaChr_SetHeadScale },
		{ "setNeckHeight", luaChr_SetNeckHeight },
		{ "setNeckForward", luaChr_SetNeckForward },
		{ "setNeckRight", luaChr_SetNeckRight },
		{ "setHandForward", luaChr_SetHandForward },
		{ "setHandRight", luaChr_SetHandRight },
		{ "setShoulderForward", luaChr_SetShoulderForward },
		{ "setShoulderRight", luaChr_SetShoulderRight },
		{ "setToolForward", luaChr_SetToolForward },
		{ "setToolRight", luaChr_SetToolRight },
		{ "setToolScale", luaChr_SetToolScale },
		{ "setShoulderScale", luaChr_SetShoulderScale },
		{ "setHeadHeight", luaChr_SetHeadHeight },
		{ "setFootRight", luaChr_SetFootRight },
		{ "setChestHeight", luaChr_SetChestHeight },
		{ "setBeltHeight", luaChr_SetBeltHeight },
		{ "setPantsHeight", luaChr_SetPantsHeight },
		{ "setInvisibleLegHeight", luaChr_SetInvisibleLegHeight },
		{ "setFootHeight", luaChr_SetFootHeight },
		{ "setOrigin", luaChr_SetOrigin },
		{ "setHipOffset", luaChr_SetHipOffset },
		{ "setJumpTimeFactor", luaChr_SetJumpTimeFactor },
		{ "setRunTimeFactor", luaChr_SetRunTimeFactor },
		{ "setIdleTimeFactor", luaChr_SetIdleTimeFactor },
		{ nullptr, nullptr }
	};
	static_assert(lengthof(funcs) - 9 == lengthof(ChrSkeletonAttributeMetaArray), "Array sizes should match");

	lua::LUA lua;
	lua.reg("chr", funcs);

	if (!lua.load(luaString)) {
		Log::error("%s", lua.error().c_str());
		return false;
	}

	lua.newGlobalData<CharacterSettings>("Settings", &settings);
	if (!lua.execute("init")) {
		Log::error("%s", lua.error().c_str());
		return false;
	}

	settings.update();

	return true;
}

bool CharacterSettings::update() {
	if (!skeletonAttr.update()) {
		return false;
	}
	const char* racePath = race.c_str();
	const char* genderPath = gender.c_str();

	if (!core::string::formatBuf(basePath, sizeof(basePath), "models/characters/%s/%s", racePath, genderPath)) {
		Log::error("Failed to initialize the character path buffer. Can't load models.");
		return false;
	}

	// TODO: integrate a static_assert here
	paths[std::enum_value(CharacterMeshType::Head)]     = &head;
	paths[std::enum_value(CharacterMeshType::Chest)]    = &chest;
	paths[std::enum_value(CharacterMeshType::Belt)]     = &belt;
	paths[std::enum_value(CharacterMeshType::Pants)]    = &pants;
	paths[std::enum_value(CharacterMeshType::Hand)]     = &hand;
	paths[std::enum_value(CharacterMeshType::Foot)]     = &foot;
	paths[std::enum_value(CharacterMeshType::Shoulder)] = &shoulder;
	paths[std::enum_value(CharacterMeshType::Glider)]   = nullptr;

	return true;
}

CharacterSettings::CharacterSettings() {
	for (size_t i = 0; i < paths.size(); ++i) {
		paths[i] = nullptr;
	}
}

std::string CharacterSettings::fullPath(CharacterMeshType type, const char* name) const {
	const std::string& p = path(type, name);
	return core::string::format("%s/%s.vox", basePath, p.c_str());
}

std::string CharacterSettings::fullPath(CharacterMeshType type) const {
	return core::string::format("%s/%s.vox", basePath, path(type));
}

const char* CharacterSettings::path(CharacterMeshType type) const {
	if (paths[std::enum_value(type)] == nullptr) {
		return "";
	}
	return paths[std::enum_value(type)]->c_str();
}

std::string CharacterSettings::path(CharacterMeshType type, const char *name) const {
	return core::string::format("%s/%s", animation::toString(type), name);
}

void CharacterSettings::copyFrom(const CharacterSettings& other) {
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
	for (size_t i = 0; i < paths.size(); ++i) {
		paths[i] = nullptr;
	}
	memcpy(basePath, other.basePath, sizeof(basePath));
	update();
}

void CharacterSettings::setRace(const char *str) {
	race = str;
}

void CharacterSettings::setGender(const char *str) {
	gender = str;
}

void CharacterSettings::setChest(const char *str) {
	chest = str;
}

void CharacterSettings::setBelt(const char *str) {
	belt = str;
}

void CharacterSettings::setPants(const char *str) {
	pants = str;
}

void CharacterSettings::setHand(const char *str) {
	hand = str;
}

void CharacterSettings::setFoot(const char *str) {
	foot = str;
}

void CharacterSettings::setHead(const char *str) {
	head = str;
}

void CharacterSettings::setShoulder(const char *str) {
	shoulder = str;
}


}
