/**
 * @file
 */

#pragma once

#include <stddef.h>

namespace animation {

/**
 * @brief Meta structure for iterating over the float values of a skeleton attribute structure
 */
struct SkeletonAttributeMeta {
	const char *name;
	size_t offset;
};

}
