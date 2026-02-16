/**
 * @file
 */

#pragma once

#include "color/Color.h"
#include "core/Common.h"
#include "core/Enum.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "core/tests/TestHelper.h"
#include "core/tests/TestColorHelper.h"
#include "voxel/tests/VoxelPrinter.h"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/SceneGraphNode.h"
#include <gtest/gtest.h>

namespace glm {
::std::ostream &operator<<(::std::ostream &os, const mat4x4 &matrix);
::std::ostream &operator<<(::std::ostream &os, const mat3x3 &matrix);
::std::ostream &operator<<(::std::ostream &os, const mat4x3 &matrix);
}

namespace palette {
::std::ostream &operator<<(::std::ostream &os, const palette::Palette &palette);
::std::ostream &operator<<(::std::ostream &os, const palette::Material &material);
}

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

	SceneGraphModels = 2048, // disable this for single volume formats

	Transform = Animations | Scale | Pivot | Translation,
	All = Palette | Color | Transform | SceneGraphModels,										   // no region here
	Mesh = Color | Animations | Scale | Pivot | Translation | SceneGraphModels | IgnoreHollow,
	AllPaletteMinMatchingColors = PaletteMinMatchingColors | Color | Transform | SceneGraphModels, // no region here
	AllPaletteColorOrderDiffers = PaletteColorOrderDiffers | Color | Transform | SceneGraphModels, // no region here
	AllPaletteColorsScaled = PaletteColorsScaled | Color | Transform | SceneGraphModels,		   // no region here
	Max
};
CORE_ENUM_BIT_OPERATIONS(ValidateFlags);

void partialPaletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, voxel::ValidateFlags flags, float maxDelta);
void paletteComparatorScaled(const palette::Palette &pal1, const palette::Palette &pal2, int maxDelta = 4);
void orderPaletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, float maxDelta = 0.001f);
void paletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, float maxDelta = 0.001f);
void colorComparator(const palette::Palette &pal1, const palette::Palette &pal2, color::RGBA c1, color::RGBA c2, uint8_t palIdx, float maxDelta = 0.001f);
void colorComparator(color::RGBA c1, color::RGBA c2, int maxDelta = 1);
void colorComparatorDistance(color::RGBA c1, color::RGBA c2, float maxDelta = 0.001f);
void keyFrameComparator(const scenegraph::SceneGraphKeyFrames &keyframes1, const scenegraph::SceneGraphKeyFrames &keyframes2, ValidateFlags flags);
void volumeComparator(const voxel::RawVolume& volume1, const palette::Palette &pal1, const voxel::RawVolume& volume2, const palette::Palette &pal2, ValidateFlags flags, float maxDelta = 0.001f);
void sceneGraphComparator(const scenegraph::SceneGraph &graph1, const scenegraph::SceneGraph &graph2, ValidateFlags flags, float maxDelta = 0.001f);
void materialComparator(const scenegraph::SceneGraph &graph1, const scenegraph::SceneGraph &graph2);
void materialComparator(const palette::Palette &pal1, const palette::Palette &pal2);

}
