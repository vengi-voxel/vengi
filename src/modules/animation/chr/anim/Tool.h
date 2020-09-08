/**
 * @file
 */

#pragma once

#include "animation/chr/CharacterSkeleton.h"
#include "animation/ToolAnimationType.h"

extern "C" void animation_chr_tool_update(double animTime, animation::ToolAnimationType animation, animation::CharacterSkeleton* skeleton, const animation::CharacterSkeletonAttribute* skeletonAttr);
