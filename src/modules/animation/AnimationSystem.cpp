/**
 * @file
 */

#include "AnimationSystem.h"
#include "app/App.h"
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
#include "io/Filesystem.h"
#include <SDL.h>

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

#if HOT_RELOAD_ANIM == 1
#define HOT_RELOAD_FUNC(name)                                                                                          \
	name = (animation_##name##_PROC *)SDL_LoadFunction(_obj, "animation_" CORE_STRINGIFY(name));                       \
	if (name == nullptr) {                                                                                             \
		Log::error("%s", SDL_GetError());                                                                              \
		return false;                                                                                                  \
	} else {                                                                                                           \
		Log::debug("Loaded symbol animation_" CORE_STRINGIFY(name) " from %p", _obj);                                  \
	}
#else
#define HOT_RELOAD_FUNC(name) name = animation_##name;
#endif

bool AnimationSystem::loadSymbols() {
#if HOT_RELOAD_ANIM == 1
#ifdef __WINDOWS__
#define SHARED_LIB_SUFFIX "dll"
#elif __MACOSX__
#define SHARED_LIB_SUFFIX "dylib"
#else
#define SHARED_LIB_SUFFIX "so"
#endif
	// TODO: should be in construct()
	const core::VarPtr animLibPath = core::Var::get("anim_lib", "libanim." SHARED_LIB_SUFFIX);
	const core::String& libName = animLibPath->strVal();
	const char *searchPaths[] = {
		libName.c_str(),
#ifdef HOT_RELOAD_LIB
		HOT_RELOAD_LIB,
#endif
		nullptr
	};
	void *newObj = nullptr;
	const char *finalLibName = nullptr;
	for (const char **searchPath = searchPaths; *searchPath; ++searchPath) {
		newObj = SDL_LoadObject(*searchPath);
		if (newObj != nullptr) {
			Log::debug("Attempt to load %s", *searchPath);
			finalLibName = *searchPath;
			break;
		}
		Log::debug("Failed to load %s", *searchPath);
	}
	if (newObj == nullptr) {
		return false;
	}
	SDL_UnloadObject(_obj);
	SDL_UnloadObject(newObj);
	animLibPath->setVal(finalLibName);
	_obj = SDL_LoadObject(finalLibName);
#endif
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

bool AnimationSystem::init() {
	if (!loadSymbols()) {
#if HOT_RELOAD_ANIM == 1
		const core::String& libName = core::Var::getSafe("anim_lib")->strVal();
		Log::error("Failed to load the animation symbols. Make sure the cvar anim_lib points to the library: %s", libName.c_str());
#endif
		return false;
	}
#if HOT_RELOAD_ANIM == 1
	const core::String& libName = core::Var::getSafe("anim_lib")->strVal();
	static io::FileWatcher watcher { this, [] (void* userdata, const char *file) {
		AnimationSystem* as = (AnimationSystem*)userdata;
		Log::info("Reloading animation lib %s", file);
		int retryCount = 0;
		while (!as->loadSymbols()) {
			++retryCount;
			if (retryCount > 100) {
				Log::error("Failed to reload - aborting");
				return;
			}
			Log::warn("Failed to reload - retrying");
			SDL_Delay(10);
		}
		Log::info("Reloaded animation lib");
	}};
	io::filesystem()->watch(libName, &watcher);
#endif
	return true;
}

#undef HOT_RELOAD_FUNC

void AnimationSystem::shutdown() {
#if HOT_RELOAD_ANIM == 1
	SDL_UnloadObject(_obj);
	_obj = nullptr;
	const core::VarPtr& animLib = core::Var::get("anim_lib", nullptr);
	if (animLib) {
		const core::String& libName = animLib->strVal();
		io::filesystem()->unwatch(libName);
	}
#endif
}

}
