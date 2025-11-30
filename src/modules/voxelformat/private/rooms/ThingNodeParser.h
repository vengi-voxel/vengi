/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include <glm/vec3.hpp>

namespace core {
class Tokenizer;
} // namespace core

namespace voxelformat {

struct MediaCanvas {
	int mediaStartTime = 0;
	int mediaVolume = 0;
	glm::vec3 localPos{0.0f};
	glm::vec3 localRot{0.0f};
	glm::vec3 localScale{0.0f};
};

struct AnimSpec {
	int mode = 0;
	int startFrame = 0;
	int endFrame = 0;
	int fps = 0;
	int pause = 0;
};

struct RenderSpec {
	float glowThresh = 0;
	float glowIntensity = 0;
};

struct NodeSpec {
	core::String name;
	core::String modelName;
	core::String thingLibraryId;
	core::String mediaName; // image that should get added to the scene as plane
	MediaCanvas mediaCanvas;
	float opacity = 1.0f;
	glm::vec3 localPos{0.0f};
	glm::vec3 localRot{0.0f};
	glm::vec3 localSize{0.0f};
	float scale = 1.0f;
	core::RGBA color{0, 0, 0, 255};
	core::DynamicArray<NodeSpec> children;
	AnimSpec animSpec;
	RenderSpec renderSpec;
};

class ThingNodeParser {
private:
	bool parseChildren(core::Tokenizer &tok, NodeSpec &nodeSpec) const;
	bool parseNode(core::Tokenizer &tok, NodeSpec &nodeSpec) const;
public:
	bool parseNode(const core::String &string, NodeSpec &nodeSpec) const;
};

} // namespace voxelformat
