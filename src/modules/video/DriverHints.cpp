/**
 * @file
 * @brief Driver hints for using the high performance card in e.g. optimus setups
 */

#include "core/Platform.h"
#include <stdint.h>

extern "C" {
	// https://community.amd.com/external-link.jspa?url=http%3A%2F%2Fdeveloper.download.nvidia.com%2Fdevzone%2Fdevcenter%2Fgamegraphics%2Ffiles%2FOptimusRenderingPolicies.pdf
	CORE_EXPORT_SYMBOL int32_t NvOptimusEnablement = 1;
	// https://community.amd.com/thread/169965
	CORE_EXPORT_SYMBOL int32_t AmdPowerXpressRequestHighPerformance = 1;
}
