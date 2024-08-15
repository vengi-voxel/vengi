/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "voxel/Region.h"
#include <glm/vec3.hpp>

namespace voxedit {

class SceneManager;
class SelectionManager;

/**
 * @ingroup Brushes
 */
class SelectBrush : public AABBBrush {
private:
	using Super = AABBBrush;
	SelectionManager *_selectionManager;
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	SelectBrush(SelectionManager *selectionManager)
		: Super(BrushType::Select, ModifierType::Select, ModifierType::Select), _selectionManager(selectionManager) {
		setBrushClamping(true);
	}
	virtual ~SelectBrush() = default;
};

} // namespace voxedit
