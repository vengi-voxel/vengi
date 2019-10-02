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

static int luaMain_SetRace(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->setRace(luaL_checkstring(l, 1));
	return 0;
}

static int luaMain_SetGender(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->setGender(luaL_checkstring(l, 1));
	return 0;
}

static int luaMain_SetChest(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->setChest(luaL_checkstring(l, 1));
	return 0;
}

static int luaMain_SetBelt(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->setBelt(luaL_checkstring(l, 1));
	return 0;
}

static int luaMain_SetPants(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->setPants(luaL_checkstring(l, 1));
	return 0;
}

static int luaMain_SetHand(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->setHand(luaL_checkstring(l, 1));
	return 0;
}

static int luaMain_SetFoot(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->setFoot(luaL_checkstring(l, 1));
	return 0;
}

static int luaMain_SetHead(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->setHead(luaL_checkstring(l, 1));
	return 0;
}

static int luaMain_SetShoulder(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->setShoulder(luaL_checkstring(l, 1));
	return 0;
}

static int luaMain_SetScaler(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.scaler = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetHeadScale(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.headScale = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetNeckHeight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.neckHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetNeckForward(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.neckForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetNeckRight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.neckRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetHandForward(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.handForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetHandRight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.handRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetShoulderForward(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.shoulderForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetShoulderRight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.shoulderRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetToolForward(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.toolForward = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetToolRight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.toolRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetToolScale(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.toolScale = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetShoulderScale(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.shoulderScale = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetHeadHeight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.headHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetFootRight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.footRight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetChestHeight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.chestHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetBeltHeight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.beltHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetPantsHeight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.pantsHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetInvisibleLegHeight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.invisibleLegHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetFootHeight(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.footHeight = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetOrigin(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.origin = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetHipOffset(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.hipOffset = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetJumpTimeFactor(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.jumpTimeFactor = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetIdleTimeFactor(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.idleTimeFactor = luaL_checknumber(l, 1);
	return 0;
}

static int luaMain_SetRunTimeFactor(lua_State * l) {
	CharacterSettings *chrSettings = luaGetCharacterSettings(l);
	chrSettings->skeletonAttr.runTimeFactor = luaL_checknumber(l, 1);
	return 0;
}

}
