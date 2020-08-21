/**
 * @file
 */

#include "LUAAnimation.h"
#include "animation/Bone.h"
#include "animation/BoneUtil.h"
#include "animation/SkeletonAttribute.h"
#include "commonlua/LUAFunctions.h"

namespace animation {

static const char* luaanim_metaskeleton() {
	return "__meta_skeleton";
}

static const char* luaanim_metabone() {
	return "__meta_bone";
}

int luaanim_pushskeletonattributes(lua_State* s, const SkeletonAttribute &skeletonAttr) {
	int n = 0;
	for (const SkeletonAttributeMeta* metaIter = skeletonAttr.metaArray(); metaIter && metaIter->name; ++metaIter) {
		++n;
	}
	lua_createtable(s, 0, n);
	for (const SkeletonAttributeMeta* metaIter = skeletonAttr.metaArray(); metaIter && metaIter->name; ++metaIter) {
		const SkeletonAttributeMeta& meta = *metaIter;
		float *saVal = (float*)(((uint8_t*)&skeletonAttr) + meta.offset);
		lua_pushstring(s, meta.name);
		lua_pushnumber(s, *saVal);
		lua_settable(s, -3);
	}
	return 1;
}

int luaanim_pushskeleton(lua_State* s, Skeleton &skeleton) {
	return clua_pushudata(s, &skeleton, luaanim_metaskeleton());
}

static Skeleton* luaanim_toskeleton(lua_State* s, int n) {
	return *(Skeleton**)clua_getudata<Skeleton*>(s, n, luaanim_metaskeleton());
}

static int luaanim_pushbone(lua_State* s, const Bone &bone) {
	return clua_pushudata(s, &bone, luaanim_metabone());
}

static Bone* luaanim_tobone(lua_State* s, int n) {
	return *(Bone**)clua_getudata<Bone*>(s, n, luaanim_metabone());
}

static int luaanim_skeleton_bone(lua_State* s) {
	Skeleton* skeleton = luaanim_toskeleton(s, 1);
	const char* name = lua_tostring(s, 2);
	const BoneId boneId = toBoneId(name);
	if (boneId == BoneId::Max) {
		return luaL_error(s, "%s is no valid bone", name);
	}
	return luaanim_pushbone(s, skeleton->bone(boneId));
}

static int luaanim_skeleton_torsobone(lua_State* s) {
	Skeleton* skeleton = luaanim_toskeleton(s, 1);
	const float scale = lua_tonumber(s, 2);
	return luaanim_pushbone(s, skeleton->torsoBone(scale));
}

static int luaanim_bone_equal(lua_State* s) {
	const Bone* a = luaanim_tobone(s, 1);
	const Bone* b = luaanim_tobone(s, 2);
	lua_pushboolean(s, a == b);
	return 1;
}

static int luaanim_bone_tostring(lua_State* s) {
	const Bone* bone = luaanim_tobone(s, 1);
	lua_pushfstring(s, "Bone[scale: %f:%f:%f, translation: %f:%f:%f, orientation: %f:%f:%f:%f]",
		bone->scale.x, bone->scale.y, bone->scale.z,
		bone->translation.x, bone->translation.y, bone->translation.z,
		bone->orientation.x, bone->orientation.y, bone->orientation.z, bone->orientation.w
	);
	return 0;
}

static int luanim_skeleton_zerobone(lua_State* s) {
	static constexpr Bone zero = animation::zero();
	return luaanim_pushbone(s, zero);
}

static int luaanim_bone_index(lua_State* s) {
	const Bone* bone = luaanim_tobone(s, 1);
	const char* i = luaL_checkstring(s, 2);

	switch (*i) {
	case 't':
		return clua_push(s, bone->translation);
	case 's':
		return clua_push(s, bone->scale);
	case 'o': {
		return clua_push(s, bone->orientation);
	}
	}
	return luaL_error(s, "Invalid component %s - supported are translation, scale and orientation", i);
}

static int luaanim_bone_newindex(lua_State* s) {
	Bone* bone = luaanim_tobone(s, 1);
	const char *i = luaL_checkstring(s, 2);

	switch (*i) {
	case 't':
		bone->translation = *clua_get<glm::vec3>(s, 3);
		return 1;
	case 's':
		bone->scale = *clua_get<glm::vec3>(s, 3);
		return 1;
	case 'o': {
		bone->orientation = *clua_get<glm::quat>(s, 3);
		return 1;
	}
	}
	return luaL_error(s, "Invalid component %s - supported are translation, scale and orientation", i);
}

void luaanim_setup(lua_State* s) {
	static const luaL_Reg boneFuncs[] = {
		{"__eq", luaanim_bone_equal},
		{"__tostring", luaanim_bone_tostring},
		{"__index", luaanim_bone_index},
		{"__newindex", luaanim_bone_newindex},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, boneFuncs, luaanim_metabone());

	static const luaL_Reg skeletonFuncs[] = {
		{"zeroBone", luanim_skeleton_zerobone},
		{"bone", luaanim_skeleton_bone},
		{"torsoBone", luaanim_skeleton_torsobone},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, skeletonFuncs, luaanim_metaskeleton());

	clua_cmdregister(s);
	clua_varregister(s);
	clua_logregister(s);

	clua_vecregister<glm::vec2>(s);
	clua_vecregister<glm::vec3>(s);
	clua_vecregister<glm::vec4>(s);
	clua_vecregister<glm::ivec2>(s);
	clua_vecregister<glm::ivec3>(s);
	clua_vecregister<glm::ivec4>(s);
	clua_quatregister(s);
}

bool luaanim_execute(lua_State* s, const char *animation, double animTime, double velocity, Skeleton &skeleton, const SkeletonAttribute &skeletonAttr) {
	lua_getglobal(s, animation);
	if (lua_isnil(s, -1)) {
		Log::error("Function '%s' wasn't found", animation);
		return false;
	}

	lua_pushnumber(s, animTime);
	lua_pushnumber(s, velocity);
	luaanim_pushskeleton(s, skeleton);
	luaanim_pushskeletonattributes(s, skeletonAttr);
	const int ret = lua_pcall(s, 4, 0, 0);
	if (ret != LUA_OK) {
		Log::error("%s", lua_tostring(s, -1));
		return false;
	}
	return true;
}

}
