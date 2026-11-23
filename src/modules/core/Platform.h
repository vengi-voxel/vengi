/**
 * @file
 */

#pragma once

#include <SDL3/SDL_platform_defines.h>

#if defined(SDL_PLATFORM_WINDOWS) || defined(__CYGWIN__)
#define CORE_EXPORT_SYMBOL __declspec(dllexport)
#else
#define CORE_EXPORT_SYMBOL
#endif

namespace core {

constexpr bool isWebPlatform() {
#ifdef SDL_PLATFORM_EMSCRIPTEN
	return true;
#else
	return false;
#endif
}

constexpr bool isAndroidPlatform() {
#ifdef SDL_PLATFORM_ANDROID
	return true;
#else
	return false;
#endif
}

constexpr bool isMobilePlatform() {
#ifdef SDL_PLATFORM_IOS
	return true;
#else
	return isAndroidPlatform();
#endif
}

} // namespace core
