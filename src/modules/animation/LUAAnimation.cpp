/**
 * @file
 */

#include "LUAAnimation.h"
#include "animation/Bone.h"
#include "animation/BoneUtil.h"
#include "animation/SkeletonAttribute.h"
#include "commonlua/LUAFunctions.h"

namespace animation {

static const char* luaanim_metaboneutil() {
	return "__meta_boneutil";
}

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

static int luaanim_pushbone(lua_State* s, const Bone* bone) {
	return clua_pushudata(s, bone, luaanim_metabone());
}

static Bone* luaanim_tobone(lua_State* s, int n) {
	return *(Bone**)clua_getudata<Bone*>(s, n, luaanim_metabone());
}

static int luaanim_skeleton_bone(lua_State* s) {
	Skeleton* skeleton = luaanim_toskeleton(s, 1);
	const char* name = lua_tostring(s, 2);
	const BoneId boneId = toBoneId(name);
	if (boneId == BoneId::Max) {
		return clua_error(s, "%s is no valid bone", name);
	}
	return luaanim_pushbone(s, &skeleton->bone(boneId));
}

static int luaanim_skeleton_torsobone(lua_State* s) {
	Skeleton* skeleton = luaanim_toskeleton(s, 1);
	const float scale = lua_tonumber(s, 2);
	return luaanim_pushbone(s, &skeleton->torsoBone(scale));
}

#define luaanim_skeleton_bonebyid(name, id) \
	static int luaanim_skeleton_##name##bone(lua_State* s) { \
		Skeleton* skeleton = luaanim_toskeleton(s, 1); \
		return luaanim_pushbone(s, &skeleton->bone(BoneId::id)); \
	}

luaanim_skeleton_bonebyid(head, Head)
luaanim_skeleton_bonebyid(chest, Chest)
luaanim_skeleton_bonebyid(belt, Belt)
luaanim_skeleton_bonebyid(pants, Pants)
luaanim_skeleton_bonebyid(lefthand, LeftHand)
luaanim_skeleton_bonebyid(righthand, RightHand)
luaanim_skeleton_bonebyid(leftfoot, LeftFoot)
luaanim_skeleton_bonebyid(rightfoot, RightFoot)
luaanim_skeleton_bonebyid(tool, Tool)
luaanim_skeleton_bonebyid(leftshoulder, LeftShoulder)
luaanim_skeleton_bonebyid(rightshoulder, RightShoulder)
luaanim_skeleton_bonebyid(glider, Glider)
luaanim_skeleton_bonebyid(leftwing, LeftWing)
luaanim_skeleton_bonebyid(rightwing, RightWing)
luaanim_skeleton_bonebyid(tail, Tail)
luaanim_skeleton_bonebyid(body, Body)

static int luaanim_bone_equal(lua_State* s) {
	const Bone* a = luaanim_tobone(s, 1);
	const Bone* b = luaanim_tobone(s, 2);
	lua_pushboolean(s, a == b);
	return 1;
}

static int luaanim_bone_tostring(lua_State* s) {
	const Bone& bone = *luaanim_tobone(s, 1);
	lua_pushfstring(s, "Bone[scale: %f:%f:%f, translation: %f:%f:%f, orientation: %f:%f:%f:%f]",
		bone.scale.x, bone.scale.y, bone.scale.z,
		bone.translation.x, bone.translation.y, bone.translation.z,
		bone.orientation.x, bone.orientation.y, bone.orientation.z, bone.orientation.w
	);
	return 1;
}

static int luaanim_bone_destroy(lua_State* s) {
	return 0;
}

static int luaanim_bone_setscale(lua_State* s) {
	Bone& bone = *luaanim_tobone(s, 1);
	glm::vec3& v = bone.scale;
	v.x = lua_tonumber(s, 2);
	v.y = luaL_optnumber(s, 3, v.x);
	v.z = luaL_optnumber(s, 4, v.y);
	return 0;
}

static int luaanim_bone_settranslation(lua_State* s) {
	Bone& bone = *luaanim_tobone(s, 1);
	glm::vec3& v = bone.translation;
	v.x = lua_tonumber(s, 2);
	v.y = luaL_optnumber(s, 3, v.x);
	v.z = luaL_optnumber(s, 4, v.y);
	return 0;
}

static int luaanim_bone_setorientation(lua_State* s) {
	Bone& bone = *luaanim_tobone(s, 1);
	bone.orientation = *clua_get<glm::quat>(s, 2);
	return 0;
}

static int luaanim_skeleton_hidegliderbone(lua_State* s) {
	Skeleton* skeleton = luaanim_toskeleton(s, 1);
	skeleton->bone(BoneId::Glider) = zero();
	return 0;
}

static int luaanim_bone_zero(lua_State* s) {
	Bone& bone = *luaanim_tobone(s, 1);
	bone = animation::zero();
	return 0;
}

static int luaanim_bone_mirror_x(lua_State* s) {
	Bone& self = *luaanim_tobone(s, 1);
	const Bone& bone = *luaanim_tobone(s, 2);
	self = mirrorX(bone);
	return 0;
}

static int luaanim_bone_mirror_xyz(lua_State* s) {
	Bone& self = *luaanim_tobone(s, 1);
	const Bone& bone = *luaanim_tobone(s, 2);
	self = mirrorXYZ(bone);
	return 0;
}

static int luaanim_bone_mirror_xz(lua_State* s) {
	Bone& self = *luaanim_tobone(s, 1);
	const Bone& bone = *luaanim_tobone(s, 2);
	self = mirrorXZ(bone);
	return 0;
}

static int luaanim_boneutil_rotate_xyz(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float z = lua_tonumber(s, 2);
	return clua_push(s, rotateXZ(x, z));
}

static int luaanim_boneutil_rotate_xy(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float y = lua_tonumber(s, 2);
	return clua_push(s, rotateXY(x, y));
}

static int luaanim_boneutil_rotate_yz(lua_State* s) {
	const float y = lua_tonumber(s, 1);
	const float z = lua_tonumber(s, 2);
	return clua_push(s, rotateXY(y, z));
}

static int luaanim_boneutil_rotate_xz(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float z = lua_tonumber(s, 2);
	return clua_push(s, rotateXY(x, z));
}

static int luaanim_boneutil_rotate_x(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	return clua_push(s, rotateX(x));
}

static int luaanim_boneutil_rotate_y(lua_State* s) {
	const float y = lua_tonumber(s, 1);
	return clua_push(s, rotateY(y));
}

static int luaanim_boneutil_rotate_z(lua_State* s) {
	const float z = lua_tonumber(s, 1);
	return clua_push(s, rotateZ(z));
}

static int luaanim_boneutil_mirror_vec3_xz(lua_State* s) {
	const glm::vec3* translation = clua_get<glm::vec3>(s, 1);
	const glm::vec3& mirrored = mirrorXZ(*translation);
	return clua_push(s, mirrored);
}

void luaanim_setup(lua_State* s) {
	static const luaL_Reg boneFuncs[] = {
		{"mirrorX",        luaanim_bone_mirror_x},
		{"mirrorXYZ",      luaanim_bone_mirror_xyz},
		{"mirrorXZ",       luaanim_bone_mirror_xz},
		{"zero",           luaanim_bone_zero},
		{"__eq",           luaanim_bone_equal},
		{"__tostring",     luaanim_bone_tostring},
		{"__gc",           luaanim_bone_destroy},
		{"setScale",       luaanim_bone_setscale},
		{"setTranslation", luaanim_bone_settranslation},
		{"setOrientation", luaanim_bone_setorientation},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, boneFuncs, luaanim_metabone());

	static const luaL_Reg skeletonFuncs[] = {
		{"bone",              luaanim_skeleton_bone},
		{"headBone",          luaanim_skeleton_headbone},
		{"chestBone",         luaanim_skeleton_chestbone},
		{"beltBone",          luaanim_skeleton_beltbone},
		{"pantsBone",         luaanim_skeleton_pantsbone},
		{"lefthandBone",      luaanim_skeleton_lefthandbone},
		{"righthandBone",     luaanim_skeleton_righthandbone},
		{"leftfootBone",      luaanim_skeleton_leftfootbone},
		{"rightfootBone",     luaanim_skeleton_rightfootbone},
		{"toolBone",          luaanim_skeleton_toolbone},
		{"leftshoulderBone",  luaanim_skeleton_leftshoulderbone},
		{"rightshoulderBone", luaanim_skeleton_rightshoulderbone},
		{"gliderBone",        luaanim_skeleton_gliderbone},
		{"torsoBone",         luaanim_skeleton_torsobone},
		{"leftwingBone",      luaanim_skeleton_leftwingbone},
		{"rightwingBone",     luaanim_skeleton_rightwingbone},
		{"tailBone",          luaanim_skeleton_tailbone},
		{"bodyBone",          luaanim_skeleton_bodybone},
		{"headBone",          luaanim_skeleton_headbone},
		{"hideGliderBone",    luaanim_skeleton_hidegliderbone},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, skeletonFuncs, luaanim_metaskeleton());

	static const luaL_Reg boneutilFuncs[] = {
		{"rotateXYZ",    luaanim_boneutil_rotate_xyz},
		{"rotateXY",     luaanim_boneutil_rotate_xy},
		{"rotateYZ",     luaanim_boneutil_rotate_yz},
		{"rotateXZ",     luaanim_boneutil_rotate_xz},
		{"rotateY",      luaanim_boneutil_rotate_x},
		{"rotateX",      luaanim_boneutil_rotate_y},
		{"rotateZ",      luaanim_boneutil_rotate_z},
		{"mirrorVec3XZ", luaanim_boneutil_mirror_vec3_xz},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, boneutilFuncs, luaanim_metaboneutil(), "boneutil");
	clua_mathregister(s);
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
