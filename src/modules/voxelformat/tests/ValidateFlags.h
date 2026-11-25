/**
 * @file
 */

#pragma once

#include "core/Enum.h"

namespace voxel {

enum class ValidateFlags {
	None = 0,
	Region = 1, // deprecated
	Color = 2,

	Translation = 4,
	Pivot = 8,
	Scale = 16,

	Animations = 32,

	Palette = 64,

	IgnoreHollow = 128, // used in combination with mesh formats that got their hollows filled with generic,2 voxel

	PaletteMinMatchingColors = 256, // only check the first n colors of a palette. There are formats that always have
									// 256 colors, and if we compare those palettes to a palette of a format which also
									// supports less colors, we would get a false negative - the order of the colors
									// still has to match!

	PaletteColorOrderDiffers = 512, // colors must match, but might have a different order in the palette. This happens
									// often for rgb(a) based formats - when the coordinate system between format
									// conversions differ, the first voxel color will get the first palette entry.

	PaletteColorsScaled = 1024, // palette color count must match - but the colors might be slightly different - see
								// the maxDelta parameters in the tests

	SceneGraphModels = 2048,	   // disable this for single volume formats
	SceneGraphModelsParent = 4096, // only compare the parent node of the models - but not the children - used for
								   // formats that don't support multiple models or a scene graph

	// Occupied world cells (translation + voxel - pivot * dims). Opt-in when a
	// format encodes placement as pivot or baked TRS, so T/pivot fields differ.
	OccupiedExact = 8192,		// src and dest occupied world cells must match
	OccupiedDestSubset = 16384, // every dest occupied world cell exists in src (surface dest)

	Transform = Animations | Scale | Pivot | Translation,
	All = Palette | Color | Transform | SceneGraphModels | SceneGraphModelsParent, // no region here
	Mesh = Color | Animations | Scale | Pivot | Translation | SceneGraphModels | SceneGraphModelsParent | IgnoreHollow,
	AllPaletteMinMatchingColors = PaletteMinMatchingColors | Color | Transform | SceneGraphModels | SceneGraphModelsParent, // no region here
	AllPaletteColorOrderDiffers = PaletteColorOrderDiffers | Color | Transform | SceneGraphModels | SceneGraphModelsParent, // no region here
	AllPaletteColorsScaled = PaletteColorsScaled | Color | Transform | SceneGraphModels | SceneGraphModelsParent,		   // no region here
	Max
};
CORE_ENUM_BIT_OPERATIONS(ValidateFlags);

} // namespace voxel
