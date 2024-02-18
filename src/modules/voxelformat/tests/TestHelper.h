/**
 * @file
 */

#pragma once

#include "core/Color.h"
#include "core/Common.h"
#include "core/Enum.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "core/tests/TestHelper.h"
#include "core/tests/TestColorHelper.h"
#include "voxel/tests/VoxelPrinter.h"
#include "scenegraph/SceneGraphNode.h"
#include <gtest/gtest.h>

namespace glm {
::std::ostream &operator<<(::std::ostream &os, const mat4x4 &matrix);
::std::ostream &operator<<(::std::ostream &os, const mat3x3 &matrix);
::std::ostream &operator<<(::std::ostream &os, const mat4x3 &matrix);
::std::ostream &operator<<(::std::ostream &os, const vec2 &v);
::std::ostream &operator<<(::std::ostream &os, const vec3 &v);
::std::ostream &operator<<(::std::ostream &os, const vec4 &v);
::std::ostream &operator<<(::std::ostream &os, const ivec2 &v);
::std::ostream &operator<<(::std::ostream &os, const ivec3 &v);
::std::ostream &operator<<(::std::ostream &os, const ivec4 &v);
}

namespace palette {
::std::ostream &operator<<(::std::ostream &os, const palette::Palette &palette);
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
									// supports less colors, we would get a false negative

	PaletteColorOrderDiffers = 512, // colors must match, but might have a different order in the palette. This happens
									// often for rgb(a) based formats - when the coordinate system between format
									// conversions differ, the first voxel color will get the first palette entry.

	PaletteColorsScaled = 1024,

	Transform = Animations | Scale | Pivot | Translation,
	All = Palette | Color | Transform,											// no region here
	AllPaletteMinMatchingColors = PaletteMinMatchingColors | Color | Transform, // no region here
	AllPaletteColorOrderDiffers = PaletteColorOrderDiffers | Color | Transform, // no region here
	AllPaletteColorsScaled = PaletteColorsScaled | Color | Transform,			// no region here
	Max
};
CORE_ENUM_BIT_OPERATIONS(ValidateFlags);

int countVoxels(const voxel::RawVolume& volume, const voxel::Voxel &voxel);
int countVoxels(const voxel::RawVolume &volume);
void partialPaletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, float maxDelta = 0.001f);
void paletteComparatorScaled(const palette::Palette &pal1, const palette::Palette &pal2, int maxDelta = 4);
void orderPaletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, float maxDelta = 0.001f);
void paletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, float maxDelta = 0.001f);
void keyFrameComparator(const scenegraph::SceneGraphKeyFrames &keyframes1, const scenegraph::SceneGraphKeyFrames &keyframes2, ValidateFlags flags);
void volumeComparator(const voxel::RawVolume& volume1, const palette::Palette &pal1, const voxel::RawVolume& volume2, const palette::Palette &pal2, ValidateFlags flags, float maxDelta = 0.001f);
void sceneGraphComparator(const scenegraph::SceneGraph &graph1, const scenegraph::SceneGraph &graph2, ValidateFlags flags, float maxDelta = 0.001f);

}
