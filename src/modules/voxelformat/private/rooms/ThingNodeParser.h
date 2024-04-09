/**
 * @file
 */

#pragma once

#include "core/RGBA.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include <glm/vec3.hpp>

namespace core {
class Tokenizer;
} // namespace core

namespace voxelformat {

struct NodeSpec {
	core::String name;
	core::String modelName;
	core::String thingLibraryId;
	float opacity = 1.0f;
	glm::vec3 localPos{0.0f};
	glm::vec3 localRot{0.0f};
	glm::vec3 localSize{0.0f};
	core::RGBA color{0, 0, 0, 255};
	core::DynamicArray<NodeSpec> children;
	// TODO renderSpec and animSpec might be interesting, too
};

class ThingNodeParser {
private:
	bool parseChildren(core::Tokenizer &tok, NodeSpec &nodeSpec) const;
	bool parseNode(core::Tokenizer &tok, NodeSpec &nodeSpec) const;
public:
	bool parseNode(const core::String &string, NodeSpec &nodeSpec) const;
};

} // namespace voxelformat
