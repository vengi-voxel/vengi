/**
 * @file
 */

#pragma once

#include "CharacterSettings.h"
#include "commonlua/LUA.h"
#include "core/String.h"
#include "animation/LUAShared.h"

namespace animation {

static CharacterSkeletonAttribute* luaGetSkeletonAttributes(lua_State * l) {
	return lua::LUA::globalData<CharacterSkeletonAttribute>(l, "SkeletonAttributes");
}

static int luaChr_SetScaler(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->scaler = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetHeadScale(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->headScale = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetNeckHeight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->neckHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetNeckForward(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->neckForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetNeckRight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->neckRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetHandForward(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->handForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetHandRight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->handRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetShoulderForward(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->shoulderForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetShoulderRight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->shoulderRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetToolForward(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->toolForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetToolRight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->toolRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetToolScale(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->toolScale = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetShoulderScale(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->shoulderScale = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetHeadHeight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->headHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetFootRight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->footRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetChestHeight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->chestHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetBeltHeight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->beltHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetPantsHeight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->pantsHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetInvisibleLegHeight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->invisibleLegHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetFootHeight(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->footHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetOrigin(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->origin = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetHipOffset(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->hipOffset = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetJumpTimeFactor(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->jumpTimeFactor = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetIdleTimeFactor(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->idleTimeFactor = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetRunTimeFactor(lua_State * l) {
	CharacterSkeletonAttribute *skeletonAttr = luaGetSkeletonAttributes(l);
	skeletonAttr->runTimeFactor = luaL_checknumber(l, 1);
	return 0;
}

}
