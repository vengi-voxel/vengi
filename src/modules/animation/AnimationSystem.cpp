/**
 * @file
 */

#include "AnimationSystem.h"
#include "chr/anim/Idle.h"
#include "chr/anim/Jump.h"
#include "chr/anim/Run.h"
#include "chr/anim/Glide.h"
#include "chr/anim/Swim.h"
#include "chr/anim/Tool.h"
#include "chr/anim/Sit.h"
#include "animal/bird/anim/Idle.h"
#include "animal/bird/anim/Run.h"
#include "core/Log.h"
#include "core/Var.h"

namespace animation {

animation_chr_glide_update_PROC* chr_glide_update = nullptr;
animation_chr_idle_update_PROC* chr_idle_update = nullptr;
animation_chr_jump_update_PROC* chr_jump_update = nullptr;
animation_chr_run_update_PROC* chr_run_update = nullptr;
animation_chr_sit_update_PROC* chr_sit_update = nullptr;
animation_chr_swim_update_PROC* chr_swim_update = nullptr;
animation_chr_tool_update_PROC* chr_tool_update = nullptr;

animation_animal_bird_run_update_PROC* animal_bird_run_update = nullptr;
animation_animal_bird_idle_update_PROC* animal_bird_idle_update = nullptr;

#define HOT_RELOAD_FUNC(name) \
	name = animation_##name;

bool AnimationSystem::init() {
	HOT_RELOAD_FUNC(chr_glide_update);
	HOT_RELOAD_FUNC(chr_idle_update);
	HOT_RELOAD_FUNC(chr_jump_update);
	HOT_RELOAD_FUNC(chr_run_update);
	HOT_RELOAD_FUNC(chr_sit_update);
	HOT_RELOAD_FUNC(chr_swim_update);
	HOT_RELOAD_FUNC(chr_tool_update);

	HOT_RELOAD_FUNC(animal_bird_run_update);
	HOT_RELOAD_FUNC(animal_bird_idle_update);

	return true;
}

#undef HOT_RELOAD_FUNC

void AnimationSystem::shutdown() {
}

}
