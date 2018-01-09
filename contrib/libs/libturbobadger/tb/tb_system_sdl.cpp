// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_system.h"

#ifdef TB_SYSTEM_SDL

#include "tb_msg.h"
#include "tb_types.h"
#include <stdio.h>

#include <SDL.h>

#ifdef TB_RUNTIME_DEBUG_INFO

void TBDebugOut(const char *str)
{
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s", str);
}

#endif // TB_RUNTIME_DEBUG_INFO

namespace tb {

// == TBSystem ========================================

double TBSystem::GetTimeMS()
{
	Uint64 freq = SDL_GetPerformanceFrequency();
	Uint64 now = SDL_GetPerformanceCounter();
	return 1000. * ((double)now / (double)freq);
}

/** Reschedule the platform timer, or cancel it if fire_time is TB_NOT_SOON.
	If fire_time is 0, it should be fired ASAP.
	If force is true, it will ask the platform to schedule it again, even if
	the fire_time is the same as last time. */
void TBSystem::RescheduleTimer(double fire_time)
{
}

int TBSystem::GetLongClickDelayMS()
{
	return 500;
}

int TBSystem::GetPanThreshold()
{
	return 5 * GetDPI() / 96;
}

int TBSystem::GetPixelsPerLine()
{
	return 40 * GetDPI() / 96;
}

int TBSystem::GetDPI()
{
#if SDL_VERSION_ATLEAST(2,0,4)
	float ddpi;
	if (SDL_GetDisplayDPI(0, &ddpi, NULL, NULL))
	{
		return 96;
	}
	return (int)ddpi;
#else
	return 96;
#endif
}

} // namespace tb

#endif // TB_SYSTEM_SDL
