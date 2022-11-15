/**
 * @file
 */

#pragma once

#include "core/Color.h"
#include "core/Common.h"
#include "core/Enum.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "core/tests/TestHelper.h"
#include "voxel/tests/VoxelPrinter.h"
#include "voxelformat/SceneGraphNode.h"
#include <gtest/gtest.h>

namespace glm {
::std::ostream &operator<<(::std::ostream &os, const mat4x4 &matrix);
::std::ostream &operator<<(::std::ostream &os, const mat3x3 &matrix);
::std::ostream &operator<<(::std::ostream &os, const vec2 &v);
::std::ostream &operator<<(::std::ostream &os, const vec3 &v);
::std::ostream &operator<<(::std::ostream &os, const vec4 &v);
::std::ostream &operator<<(::std::ostream &os, const ivec2 &v);
::std::ostream &operator<<(::std::ostream &os, const ivec3 &v);
::std::ostream &operator<<(::std::ostream &os, const ivec4 &v);
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

	Transform = Animations | Scale | Pivot | Translation,
	All = Palette | Color | Transform, // no region here

	Max
};
CORE_ENUM_BIT_OPERATIONS(ValidateFlags);

int countVoxels(const voxel::RawVolume& volume, const voxel::Voxel &voxel);
void paletteComparator(const voxel::Palette &pal1, const voxel::Palette &pal2, float maxDelta = 0.001f);
void keyFrameComparator(const voxelformat::SceneGraphKeyFrames &keyframes1, const voxelformat::SceneGraphKeyFrames &keyframes2, ValidateFlags flags);
void volumeComparator(const voxel::RawVolume& volume1, const voxel::Palette &pal1, const voxel::RawVolume& volume2, const voxel::Palette &pal2, ValidateFlags flags, float maxDelta = 0.001f);
void sceneGraphComparator(const voxelformat::SceneGraph &graph1, const voxelformat::SceneGraph &graph2, ValidateFlags flags, float maxDelta = 0.001f);

}
