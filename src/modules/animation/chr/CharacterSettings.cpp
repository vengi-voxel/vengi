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

bool loadCharacterSettings(const std::string& luaString, CharacterSettings& settings) {
	if (luaString.empty()) {
		Log::warn("empty character settings can't get loaded");
		return false;
	}
	// also change the voxel editor lua script saving
	static const luaL_Reg chrFuncs[] = {
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
	static_assert(lengthof(chrFuncs) - 9 == lengthof(ChrSkeletonAttributeMetaArray), "Array sizes should match");

	static const luaL_Reg boneFuncs[] = {
		{ "setup", luaanim_bonesetup<CharacterSettings> },
		{ nullptr, nullptr }
	};

	lua::LUA lua;
	lua.reg("chr", chrFuncs);
	lua.reg("bone", boneFuncs);
	luaanim_boneidsregister(lua.state());

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
	basePath = core::string::format("models/characters/%s/%s", racePath, genderPath);
	return true;
}

}
