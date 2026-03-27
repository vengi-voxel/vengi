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
 * Click to set the start point, click again to set the end point.
 * The measurement persists until a new start point is set by clicking again.
 * When using the reference position, the start is always the reference position
 * and each click updates the end position.
 *
 * @ingroup Brushes
 */
class RulerBrush : public Brush {
private:
	using Super = Brush;

protected:
	enum class State { Idle, Tracking, Measured };
	State _state = State::Idle;
	bool _useReferencePos = false;
	glm::ivec3 _startPos{0};
	glm::ivec3 _endPos{0};

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override {
	}

public:
	RulerBrush() : Super(BrushType::Ruler, ModifierType::Place, ModifierType::Place) {
	}
	virtual ~RulerBrush() = default;

	void construct() override;
	void shutdown() override;
	bool beginBrush(const BrushContext &ctx) override;
	void update(const BrushContext &ctx, double nowSeconds) override;
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

	bool useReferencePos() const;
	void setUseReferencePos(bool use);
};

inline const glm::ivec3 &RulerBrush::startPos() const {
	return _startPos;
}

inline const glm::ivec3 &RulerBrush::endPos() const {
	return _endPos;
}

inline bool RulerBrush::hasMeasurement() const {
	return _state != State::Idle;
}

inline glm::ivec3 RulerBrush::delta() const {
	return glm::abs(_endPos - _startPos);
}

inline bool RulerBrush::useReferencePos() const {
	return _useReferencePos;
}

inline void RulerBrush::setUseReferencePos(bool use) {
	_useReferencePos = use;
	_state = State::Idle;
}

} // namespace voxedit
