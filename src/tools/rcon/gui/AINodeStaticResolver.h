/**
 * @file
 */
#pragma once

#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"

namespace ai {
class AIStateNodeStatic;
}

namespace ai {
namespace debug {

class AINodeStaticResolver {
private:
	core::DynamicArray<ai::AIStateNodeStatic> _data;
	core::Map<int32_t, const ai::AIStateNodeStatic*> _hash;
public:
	AINodeStaticResolver();

	void set(const core::DynamicArray<ai::AIStateNodeStatic>& data);
	const ai::AIStateNodeStatic& get(int32_t id) const;
};

}
}
