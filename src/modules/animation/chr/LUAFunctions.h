/**
 * @file
 */

#pragma once

#include "CharacterSettings.h"
#include "commonlua/LUA.h"
#include "core/String.h"

namespace animation {

static CharacterSettings* luaGetCharacterSettings(lua_State * l) {
	return lua::LUA::globalData<CharacterSettings>(l, "Settings");
}

static int luaChr_SetRace(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->setRace(luaL_checkstring(l, 1));
	return 0;
}

static int luaChr_SetGender(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->setGender(luaL_checkstring(l, 1));
	return 0;
}

static int luaChr_SetChest(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->setChest(luaL_checkstring(l, 1));
	return 0;
}

static int luaChr_SetBelt(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->setBelt(luaL_checkstring(l, 1));
	return 0;
}

static int luaChr_SetPants(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->setPants(luaL_checkstring(l, 1));
	return 0;
}

static int luaChr_SetHand(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->setHand(luaL_checkstring(l, 1));
	return 0;
}

static int luaChr_SetFoot(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->setFoot(luaL_checkstring(l, 1));
	return 0;
}

static int luaChr_SetHead(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->setHead(luaL_checkstring(l, 1));
	return 0;
}

static int luaChr_SetShoulder(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->setShoulder(luaL_checkstring(l, 1));
	return 0;
}

static int luaChr_SetScaler(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.scaler = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetHeadScale(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.headScale = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetNeckHeight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.neckHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetNeckForward(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.neckForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetNeckRight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.neckRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetHandForward(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.handForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetHandRight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.handRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetShoulderForward(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.shoulderForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetShoulderRight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.shoulderRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetToolForward(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.toolForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetToolRight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.toolRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetToolScale(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.toolScale = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetShoulderScale(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.shoulderScale = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetHeadHeight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.headHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetFootRight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.footRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetChestHeight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.chestHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetBeltHeight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.beltHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetPantsHeight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.pantsHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetInvisibleLegHeight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.invisibleLegHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetFootHeight(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.footHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetOrigin(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.origin = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetHipOffset(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.hipOffset = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetJumpTimeFactor(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.jumpTimeFactor = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetIdleTimeFactor(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.idleTimeFactor = luaL_checknumber(l, 1);
	return 0;
}

static int luaChr_SetRunTimeFactor(lua_State * l) {
	CharacterSettings *settings = luaGetCharacterSettings(l);
	settings->skeletonAttr.runTimeFactor = luaL_checknumber(l, 1);
	return 0;
}

}
