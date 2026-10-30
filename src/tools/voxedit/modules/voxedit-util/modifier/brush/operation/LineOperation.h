/**
 * @file
 */
#pragma once

#include "voxedit-util/modifier/brush/LineBrush.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelgenerator/ShapeGenerator.h"

namespace voxedit {

struct LineParameters {
	core::DynamicArray<math::BezierSegment> segments;
	LineStipplePattern stipple;
	int thickness = 1;
	voxel::Voxel voxel;
};

/** Evaluates explicit control points without an input device or mutable brush. */
class LineOperation {
private:
	LineParameters _parameters;
	bool _prepared = false;

public:
	using Parameters = LineParameters;
	static constexpr bool requiresSelection = false;
	static bool sameOperation(const Parameters &, const Parameters &) {
		return true;
	}
	bool setParameters(const Parameters &parameters) {
		if (parameters.thickness < 1 || parameters.thickness > 64) {
			return false;
		}
		_parameters = parameters;
		return true;
	}
	const Parameters &parameters() const {
		return _parameters;
	}
	void prepare(const voxel::RawVolume *source, const voxel::Region &) {
		_prepared = source != nullptr;
	}
	bool hasSnapshot() const {
		return _prepared;
	}
	void evaluate(ModifierVolumeWrapper &wrapper) {
		for (const auto &segment : _parameters.segments) {
			voxelgenerator::shape::drawBezierSegment(wrapper, segment, _parameters.voxel, _parameters.stipple,
													 _parameters.thickness);
		}
	}
};

} // namespace voxedit
