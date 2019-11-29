/**
 * @file
 */

#include "CharacterSettings.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Array.h"
#include "commonlua/LUA.h"
#include "animation/LUAShared.h"

namespace animation {

bool loadCharacterSettings(const std::string& luaString, CharacterSettings& settings) {
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

	for (const animation::SkeletonAttributeMeta* metaIter = animation::ChrSkeletonAttributeMetaArray; metaIter->name; ++metaIter) {
		const animation::SkeletonAttributeMeta& meta = *metaIter;
		float *saVal = (float*)(((char*)&settings.skeletonAttr) + meta.offset);
		if (lua.valueFloatFromTable(meta.name, saVal)) {
			Log::info("Value for %s: %f", meta.name, *saVal);
		} else {
			Log::info("Value for %s not given - use default: %f", meta.name, *saVal);
		}
	}

	return settings.skeletonAttr.init();
}

}
