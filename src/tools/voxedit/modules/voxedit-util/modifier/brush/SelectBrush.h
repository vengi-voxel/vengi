/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "voxel/Region.h"
#include <glm/vec3.hpp>

namespace voxedit {

class SceneManager;

/**
 * @ingroup Brushes
 */
class SelectBrush : public AABBBrush {
private:
	using Super = AABBBrush;
	bool _remove = false;

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	SelectBrush()
		: Super(BrushType::Select, ModifierType::Select, ModifierType::Select) {
		setBrushClamping(true);
	}
	virtual ~SelectBrush() = default;

	void setRemove(bool remove) {
		_remove = remove;
	}

	bool remove() const {
		return _remove;
	}
};

} // namespace voxedit
