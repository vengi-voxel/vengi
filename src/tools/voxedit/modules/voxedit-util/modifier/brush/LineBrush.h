/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "LineState.h"
#include "core/collection/BitSet.h"
#include "voxel/Region.h"

namespace voxedit {

using LineStipplePattern = core::BitSet<9>;

/**
 * @ingroup Brushes
 */
class LineBrush : public Brush {
private:
	using Super = Brush;

protected:
	LineState _state;
	bool _continuous = false;
	LineStipplePattern _stipplePattern;
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	LineBrush() : Super(BrushType::Line) {
		for (int i = 0; i < _stipplePattern.bits(); ++i) {
			_stipplePattern.set(i, true);
		}
	}
	virtual ~LineBrush() = default;
	void construct() override;
	void update(const BrushContext &ctx, double nowSeconds) override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;
	void endBrush(BrushContext &ctx) override;

	void reset() override;
	bool continuous() const;
	void setContinuous(bool continuous);

	void setStippleBit(int index, bool value);
	LineStipplePattern &stipplePattern();
};

inline bool LineBrush::continuous() const {
	return _continuous;
}

inline void LineBrush::setContinuous(bool continuous) {
	_continuous = continuous;
}

inline LineStipplePattern &LineBrush::stipplePattern() {
	return _stipplePattern;
}

inline void LineBrush::setStippleBit(int index, bool value) {
	_stipplePattern.set(index, value);
}

} // namespace voxedit
