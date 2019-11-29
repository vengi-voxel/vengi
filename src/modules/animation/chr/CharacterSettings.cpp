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
	static const luaL_Reg skeletonFuncs[] = {
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
	static_assert(lengthof(skeletonFuncs) == lengthof(ChrSkeletonAttributeMetaArray), "Array sizes should match");

	if (luaString.empty()) {
		Log::warn("empty animation settings can't get loaded");
		return false;
	}

	lua::LUA lua;
	lua.reg("skeleton", skeletonFuncs);
	lua.reg("settings", settingsFuncs);
	lua.reg("bone", boneFuncs);
	luaanim_boneidsregister(lua.state());

	if (!lua.load(luaString)) {
		Log::error("%s", lua.error().c_str());
		return false;
	}

	lua.newGlobalData<AnimationSettings>("Settings", &settings);
	lua.newGlobalData<CharacterSkeletonAttribute>("SkeletonAttributes", &settings.skeletonAttr);
	if (!lua.execute("init")) {
		Log::error("%s", lua.error().c_str());
		return false;
	}

	return settings.skeletonAttr.init();
}

}
