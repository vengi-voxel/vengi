/**
 * @file
 */

#pragma once

#include "animation/Skeleton.h"
#include "commonlua/LUA.h"
#include "animation/AnimationSettings.h"

namespace animation {

extern void luaanim_setup(lua_State* s);
extern int luaanim_pushskeletonattributes(lua_State* s, const SkeletonAttribute &skeletonAttr);
extern int luaanim_pushskeleton(lua_State* s, Skeleton &skeleton);
extern bool luaanim_execute(lua_State* s, const char *animation, double animTime, double velocity, Skeleton &skeleton, const SkeletonAttribute &skeletonAttr);

}
