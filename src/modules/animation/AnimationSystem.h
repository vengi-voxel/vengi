/**
 * @file
 */

#pragma once

#include "animation/ToolAnimationType.h"
#include "core/IComponent.h"

#ifndef APIENTRY
#ifdef _MSC_VER
#define APIENTRY __declspec(dllexport)
#else
#define APIENTRY
#endif
#endif

namespace animation {

class CharacterSkeleton;
class BirdSkeleton;
struct BirdSkeletonAttribute;
struct CharacterSkeletonAttribute;

typedef void (APIENTRY animation_chr_glide_update_PROC)(double animTime, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (APIENTRY animation_chr_idle_update_PROC)(double animTime, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (APIENTRY animation_chr_jump_update_PROC)(double animTime, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (APIENTRY animation_chr_run_update_PROC)(double animTime, double velocity, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (APIENTRY animation_chr_sit_update_PROC)(double animTime, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (APIENTRY animation_chr_swim_update_PROC)(double animTime, double velocity, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (APIENTRY animation_chr_tool_update_PROC)(double animTime, ToolAnimationType animation, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);

typedef void (APIENTRY animation_animal_bird_run_update_PROC)(double animTime, double velocity, BirdSkeleton* skeleton, const BirdSkeletonAttribute* skeletonAttr);
typedef void (APIENTRY animation_animal_bird_idle_update_PROC)(double animTime, BirdSkeleton* skeleton, const BirdSkeletonAttribute* skeletonAttr);

extern animation_chr_glide_update_PROC* chr_glide_update;
extern animation_chr_idle_update_PROC* chr_idle_update;
extern animation_chr_jump_update_PROC* chr_jump_update;
extern animation_chr_run_update_PROC* chr_run_update;
extern animation_chr_sit_update_PROC* chr_sit_update;
extern animation_chr_swim_update_PROC* chr_swim_update;
extern animation_chr_tool_update_PROC* chr_tool_update;
extern animation_animal_bird_run_update_PROC* animal_bird_run_update;
extern animation_animal_bird_idle_update_PROC* animal_bird_idle_update;

class AnimationSystem : public core::IComponent {
private:
	bool loadSymbols();
public:
	bool init() override;
	void shutdown() override;
};

}
