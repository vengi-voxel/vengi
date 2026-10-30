/**
 * @file
 */

#pragma once

#include "voxedit-util/modifier/ShapeType.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxedit {

class ModifierVolumeWrapper;

struct ShapeSettings {
	ShapeType shape = ShapeType::Box;
	int thickness = 1;
};

struct ShapeParameters {
	voxel::Region region = voxel::Region::InvalidRegion;
	voxel::FaceNames face = voxel::FaceNames::PositiveX;
	voxel::Voxel voxel;
};

/** Stateless evaluator shared by interactive, preview, and headless execution. */
bool evaluateShape(const ShapeSettings &settings, const ShapeParameters &parameters, ModifierVolumeWrapper &wrapper);

} // namespace voxedit
