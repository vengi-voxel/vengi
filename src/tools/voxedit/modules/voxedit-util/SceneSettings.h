/**
 * @file
 */

#pragma once

#include <glm/vec3.hpp>

namespace voxedit {

class SceneSettings {
public:
	glm::vec3 diffuseColor;
	glm::vec3 ambientColor;
	glm::vec3 sunPosition;
	glm::vec3 sunDirection;

	bool diffuseDirty = false;
	bool ambientDirty = false;
	bool sunPositionDirty = false;
	bool sunDirectionDirty = false;
	bool backgroundsDirty = false;
};

}
