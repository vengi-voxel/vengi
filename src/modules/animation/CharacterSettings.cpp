/**
 * @file
 */

#include "CharacterSettings.h"
#include "core/Log.h"
#include "commonlua/LUA.h"
#include "LUAFunctions.h"

namespace animation {

bool loadCharacterSettings(const std::string& luaString, CharacterSettings& settings) {
	if (luaString.empty()) {
		return true;
	}
	static const luaL_Reg funcs[] = {
		{ "setRace", luaMain_SetRace },
		{ "setGender", luaMain_SetGender },
		{ "setChest", luaMain_SetChest },
		{ "setBelt", luaMain_SetBelt },
		{ "setPants", luaMain_SetPants },
		{ "setHand", luaMain_SetHand },
		{ "setFoot", luaMain_SetFoot },
		{ "setHead", luaMain_SetHead },
		{ "setShoulder", luaMain_SetShoulder },
		{ "setScaler", luaMain_SetScaler },
		{ "setHeadScale", luaMain_SetHeadScale },
		{ "setNeckHeight", luaMain_SetNeckHeight },
		{ "setNeckForward", luaMain_SetNeckForward },
		{ "setNeckRight", luaMain_SetNeckRight },
		{ "setHandForward", luaMain_SetHandForward },
		{ "setHandRight", luaMain_SetHandRight },
		{ "setShoulderForward", luaMain_SetShoulderForward },
		{ "setShoulderRight", luaMain_SetShoulderRight },
		{ "setToolForward", luaMain_SetToolForward },
		{ "setToolRight", luaMain_SetToolRight },
		{ "setToolScale", luaMain_SetToolScale },
		{ "setShoulderScale", luaMain_SetShoulderScale },
		{ "setHeadHeight", luaMain_SetHeadHeight },
		{ "setFootRight", luaMain_SetFootRight },
		{ "setChestHeight", luaMain_SetChestHeight },
		{ "setBeltHeight", luaMain_SetBeltHeight },
		{ "setPantsHeight", luaMain_SetPantsHeight },
		{ "setInvisibleLegHeight", luaMain_SetInvisibleLegHeight },
		{ "setFootHeight", luaMain_SetFootHeight },
		{ "setOrigin", luaMain_SetOrigin },
		{ "setHipOffset", luaMain_SetHipOffset },
		{ "setJumpTimeFactor", luaMain_SetJumpTimeFactor },
		{ "setRunTimeFactor", luaMain_SetRunTimeFactor },
		{ "setIdleTimeFactor", luaMain_SetIdleTimeFactor },
		{ nullptr, nullptr }
	};

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

	settings.skeletonAttr.update();

	return true;
}

}
