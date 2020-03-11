/**
 * @file
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace animation {

/**
 * @brief Meta structure for iterating over the float values of a skeleton attribute structure
 * @ingroup Animation
 */
struct SkeletonAttributeMeta {
	const char *name;
	size_t offset;
};

#define SKELETONATTRIBUTE(clazz, member) \
		{ #member,  offsetof(clazz, member) }

#define SKELETONATTRIBUTE_END \
		{ nullptr, 0l }

enum class SkeletonAttributeType : int32_t {
	Character, Bird, Max
};

/**
 * @brief Structure that holds all the relevant skeleton attributes and makes them available to the lua scripts
 *
 * @sa SkeletonAttributeMeta
 * @sa SKELETONATTRIBUTE
 */
struct SkeletonAttribute {
	/**
	 * @param[in] _type The type of the skeleton
	 * @param[in] metaArray Null terminated (@sa SKELETONATTRIBUTE_END) array of SkeletonAttributeMeta entries
	 */
	SkeletonAttribute(SkeletonAttributeType _type, const SkeletonAttributeMeta *metaArray) :
			type(_type), _metaArray(metaArray) {
	}

	SkeletonAttributeType type;
	const SkeletonAttributeMeta* _metaArray;

	inline const SkeletonAttributeMeta* metaArray() const {
		return _metaArray;
	}
};

}
