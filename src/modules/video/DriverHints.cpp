/**
 * @file
 * @brief Driver hints for using the high performance card in e.g. optimus setups
 */

#include <stdint.h>

#if defined(WIN32) || defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
#define EXPORT_SYMBOL __declspec(dllexport)
#else
#define EXPORT_SYMBOL
#endif

extern "C" {
	// https://community.amd.com/external-link.jspa?url=http%3A%2F%2Fdeveloper.download.nvidia.com%2Fdevzone%2Fdevcenter%2Fgamegraphics%2Ffiles%2FOptimusRenderingPolicies.pdf
	EXPORT_SYMBOL int32_t NvOptimusEnablement = 1;
	// https://community.amd.com/thread/169965
	EXPORT_SYMBOL int32_t AmdPowerXpressRequestHighPerformance = 1;
}
