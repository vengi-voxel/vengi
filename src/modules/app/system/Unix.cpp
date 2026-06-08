/**
 * @file
 */

#include "System.h"

#include <SDL_cpuinfo.h>

#include <stdio.h>

#ifdef __APPLE__
#include <mach/mach.h>
#endif

namespace app {

int systemTotalMemoryMiB() {
	return SDL_GetSystemRAM();
}

double systemProcessMemoryGB() {
#if defined(__linux__)
	FILE *f = fopen("/proc/self/statm", "r");
	if (!f) {
		return 0.0;
	}
	long total = 0;
	long resident = 0;
	if (fscanf(f, "%ld %ld", &total, &resident) != 2) {
		fclose(f);
		return 0.0;
	}
	fclose(f);
	const long pageSize = sysconf(_SC_PAGESIZE);
	return (double)resident * (double)pageSize / (1024.0 * 1024.0 * 1024.0);
#elif defined(__APPLE__)
	mach_task_basic_info info;
	mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
	const kern_return_t result =
		task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count);
	if (result != KERN_SUCCESS) {
		return 0.0;
	}
	return (double)info.resident_size / (1024.0 * 1024.0 * 1024.0);
#else
	return 0.0;
#endif
}

} // namespace app
