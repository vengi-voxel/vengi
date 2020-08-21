#include "core/benchmark/AbstractBenchmark.h"
#include "animation/chr/anim/Glide.h"
#include "animation/chr/anim/Idle.h"
#include "animation/chr/anim/Jump.h"
#include "animation/chr/anim/Run.h"
#include "animation/chr/anim/Swim.h"
#include "animation/chr/anim/Tool.h"

#include "animation/animal/bird/anim/Idle.h"
#include "animation/animal/bird/anim/Run.h"

#include "animation/LUAAnimation.h"

class AnimationBenchmark: public core::AbstractBenchmark {
};

#define CHR_ANIM_BENCHMARK_DEFINE_F(name)                                                                              \
	BENCHMARK_DEFINE_F(AnimationBenchmark, chr_##name)(benchmark::State & state) {                                     \
		double animTime = 1.0;                                                                                         \
		animation::CharacterSkeleton skeleton;                                                                         \
		animation::CharacterSkeletonAttribute skeletonAttr;                                                            \
		skeletonAttr.init();                                                                                           \
		for (auto _ : state) {                                                                                         \
			animation::chr::name::update(animTime, skeleton, skeletonAttr);                                            \
		}                                                                                                              \
	}                                                                                                                  \
	BENCHMARK_REGISTER_F(AnimationBenchmark, chr_##name);

#define CHR_ANIM_TOOL_BENCHMARK_DEFINE_F(type)                                                                         \
	BENCHMARK_DEFINE_F(AnimationBenchmark, chr_tool_##type)(benchmark::State & state) {                                \
		double animTime = 1.0;                                                                                         \
		animation::CharacterSkeleton skeleton;                                                                         \
		animation::CharacterSkeletonAttribute skeletonAttr;                                                            \
		skeletonAttr.init();                                                                                           \
		for (auto _ : state) {                                                                                         \
			animation::chr::tool::update(animTime, animation::ToolAnimationType::type, skeleton, skeletonAttr);        \
		}                                                                                                              \
	}                                                                                                                  \
	BENCHMARK_REGISTER_F(AnimationBenchmark, chr_tool_##type);

#define CHR_ANIM_VELO_BENCHMARK_DEFINE_F(name, velocity)                                                               \
	BENCHMARK_DEFINE_F(AnimationBenchmark, chr_##name)(benchmark::State & state) {                                     \
		double animTime = 1.0;                                                                                         \
		animation::CharacterSkeleton skeleton;                                                                         \
		animation::CharacterSkeletonAttribute skeletonAttr;                                                            \
		skeletonAttr.init();                                                                                           \
		for (auto _ : state) {                                                                                         \
			animation::chr::name::update(animTime, velocity, skeleton, skeletonAttr);                                  \
		}                                                                                                              \
	}                                                                                                                  \
	BENCHMARK_REGISTER_F(AnimationBenchmark, chr_##name);

#define CHR_ANIM_LUA_VELO_BENCHMARK_DEFINE_F(name, velocity)                                                           \
	BENCHMARK_DEFINE_F(AnimationBenchmark, chr_lua_##name)(benchmark::State & state) {                                 \
		const core::String &script = io::filesystem()->load("animations/character.lua");                               \
		lua::LUA lua;                                                                                                  \
		animation::luaanim_setup(lua);                                                                                 \
		lua.load(script);                                                                                              \
		double animTime = 1.0;                                                                                         \
		animation::CharacterSkeleton skeleton;                                                                         \
		animation::CharacterSkeletonAttribute skeletonAttr;                                                            \
		skeletonAttr.init();                                                                                           \
		for (auto _ : state) {                                                                                         \
			animation::luaanim_execute(lua, #name, animTime, velocity, skeleton, skeletonAttr);                        \
		}                                                                                                              \
	}                                                                                                                  \
	BENCHMARK_REGISTER_F(AnimationBenchmark, chr_lua_##name);

#define ANIMAL_BIRD_ANIM_BENCHMARK_DEFINE_F(name)                                                                      \
	BENCHMARK_DEFINE_F(AnimationBenchmark, animal_bird_##name)(benchmark::State & state) {                             \
		double animTime = 1.0;                                                                                         \
		animation::BirdSkeleton skeleton;                                                                              \
		animation::BirdSkeletonAttribute skeletonAttr;                                                                 \
		skeletonAttr.init();                                                                                           \
		for (auto _ : state) {                                                                                         \
			animation::animal::bird::name::update(animTime, skeleton, skeletonAttr);                                   \
		}                                                                                                              \
	}                                                                                                                  \
	BENCHMARK_REGISTER_F(AnimationBenchmark, animal_bird_##name);

#define ANIMAL_BIRD_ANIM_VELO_BENCHMARK_DEFINE_F(name, velocity)                                                       \
	BENCHMARK_DEFINE_F(AnimationBenchmark, animalBird_##name)(benchmark::State & state) {                              \
		double animTime = 1.0;                                                                                         \
		animation::BirdSkeleton skeleton;                                                                              \
		animation::BirdSkeletonAttribute skeletonAttr;                                                                 \
		skeletonAttr.init();                                                                                           \
		for (auto _ : state) {                                                                                         \
			animation::animal::bird::name::update(animTime, velocity, skeleton, skeletonAttr);                         \
		}                                                                                                              \
	}                                                                                                                  \
	BENCHMARK_REGISTER_F(AnimationBenchmark, animalBird_##name);

CHR_ANIM_BENCHMARK_DEFINE_F(glide)
CHR_ANIM_BENCHMARK_DEFINE_F(jump)
CHR_ANIM_BENCHMARK_DEFINE_F(idle)
CHR_ANIM_VELO_BENCHMARK_DEFINE_F(run, 1.0f)
CHR_ANIM_VELO_BENCHMARK_DEFINE_F(swim, 1.0f)
CHR_ANIM_LUA_VELO_BENCHMARK_DEFINE_F(swim, 1.0f)
CHR_ANIM_TOOL_BENCHMARK_DEFINE_F(Swing)
CHR_ANIM_TOOL_BENCHMARK_DEFINE_F(Stroke)
CHR_ANIM_TOOL_BENCHMARK_DEFINE_F(Tense)
CHR_ANIM_TOOL_BENCHMARK_DEFINE_F(Twiddle)

ANIMAL_BIRD_ANIM_BENCHMARK_DEFINE_F(idle)
ANIMAL_BIRD_ANIM_VELO_BENCHMARK_DEFINE_F(run, 1.0f)

BENCHMARK_MAIN();
