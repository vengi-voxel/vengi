/**
 * @file
 */

#pragma once

#include "animation/ToolAnimationType.h"
#include "core/IComponent.h"
#include "animation-config.h"

#if HOT_RELOAD_ANIM > 0
#define ANIM_APICALL
#elif defined(_MSC_VER)
#define ANIM_APICALL __declspec(dllexport)
#elif defined(__ANDROID__)
#define ANIM_APICALL __attribute__((visibility("default")))
#else
#define ANIM_APICALL
#endif

#if defined(_MSC_VER)
#define ANIM_APIENTRY __stdcall
#else
#define ANIM_APIENTRY
#endif


namespace animation {

class CharacterSkeleton;
class BirdSkeleton;
struct BirdSkeletonAttribute;
struct CharacterSkeletonAttribute;

typedef void (ANIM_APIENTRY animation_chr_glide_update_PROC)(double animTime, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (ANIM_APIENTRY animation_chr_idle_update_PROC)(double animTime, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (ANIM_APIENTRY animation_chr_jump_update_PROC)(double animTime, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (ANIM_APIENTRY animation_chr_run_update_PROC)(double animTime, double velocity, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (ANIM_APIENTRY animation_chr_sit_update_PROC)(double animTime, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (ANIM_APIENTRY animation_chr_swim_update_PROC)(double animTime, double velocity, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);
typedef void (ANIM_APIENTRY animation_chr_tool_update_PROC)(double animTime, ToolAnimationType animation, CharacterSkeleton* skeleton, const CharacterSkeletonAttribute* skeletonAttr);

typedef void (ANIM_APIENTRY animation_animal_bird_run_update_PROC)(double animTime, double velocity, BirdSkeleton* skeleton, const BirdSkeletonAttribute* skeletonAttr);
typedef void (ANIM_APIENTRY animation_animal_bird_idle_update_PROC)(double animTime, BirdSkeleton* skeleton, const BirdSkeletonAttribute* skeletonAttr);

extern "C" ANIM_APICALL animation_chr_glide_update_PROC* chr_glide_update;
extern "C" ANIM_APICALL animation_chr_idle_update_PROC* chr_idle_update;
extern "C" ANIM_APICALL animation_chr_jump_update_PROC* chr_jump_update;
extern "C" ANIM_APICALL animation_chr_run_update_PROC* chr_run_update;
extern "C" ANIM_APICALL animation_chr_sit_update_PROC* chr_sit_update;
extern "C" ANIM_APICALL animation_chr_swim_update_PROC* chr_swim_update;
extern "C" ANIM_APICALL animation_chr_tool_update_PROC* chr_tool_update;
extern "C" ANIM_APICALL animation_animal_bird_run_update_PROC* animal_bird_run_update;
extern "C" ANIM_APICALL animation_animal_bird_idle_update_PROC* animal_bird_idle_update;

class AnimationSystem : public core::IComponent {
private:
#if HOT_RELOAD_ANIM == 1
	void *_obj = nullptr;
#endif
	bool loadSymbols();
public:
	bool init() override;
	void shutdown() override;
};

}
