/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "voxel/Region.h"
#include <glm/vec3.hpp>

namespace voxedit {

class SceneManager;

/**
 * @ingroup Brushes
 */
class SelectBrush : public Brush {
private:
	using Super = Brush;
	bool _selectStartPositionValid = false;
	glm::ivec3 _selectStartPosition{0};
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	SelectBrush() : Super(BrushType::Select, ModifierType::Select, ModifierType::Select) {
	}
	virtual ~SelectBrush() = default;
	void start(const glm::ivec3 &startPos);
	bool active() const override;
	void stop();
	voxel::Region calcRegion(const BrushContext &context) const override;
};

} // namespace voxedit
