/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "BrushGizmo.h"

namespace voxedit {

/**
 * @brief Measures the voxel distance between two points
 *
 * The RulerBrush is a non-destructive tool that draws a visual line between
 * two points and displays distance measurements. It does not modify any voxels.
 *
 * # Usage
 *
 * 1. Click to set the start point (reference position)
 * 2. Move cursor to the end point
 * 3. The brush draws a visual line and shows measurements in the BrushPanel
 *
 * @ingroup Brushes
 */
class RulerBrush : public Brush {
private:
	using Super = Brush;

protected:
	bool _active = false;
	glm::ivec3 _startPos{0};
	glm::ivec3 _endPos{0};

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override {
	}

public:
	RulerBrush() : Super(BrushType::Ruler, ModifierType::Place, ModifierType::Place) {
	}
	virtual ~RulerBrush() = default;

	bool beginBrush(const BrushContext &ctx) override;
	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) override;
	void endBrush(BrushContext &ctx) override;
	void reset() override;
	bool active() const override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;

	bool wantBrushGizmo(const BrushContext &ctx) const override;
	void brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const override;

	const glm::ivec3 &startPos() const;
	const glm::ivec3 &endPos() const;
	bool hasMeasurement() const;
	glm::ivec3 delta() const;
	float euclideanDistance() const;
	int manhattanDistance() const;
};

inline const glm::ivec3 &RulerBrush::startPos() const {
	return _startPos;
}

inline const glm::ivec3 &RulerBrush::endPos() const {
	return _endPos;
}

inline bool RulerBrush::hasMeasurement() const {
	return _active;
}

inline glm::ivec3 RulerBrush::delta() const {
	return glm::abs(_endPos - _startPos);
}

} // namespace voxedit
