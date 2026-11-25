/**
 * @file
 */

#pragma once

#include "ValidateFlags.h"
#include "color/RGBA.h"
#include "core/Common.h"
#include "core/collection/Buffer.h"
#include "palette/Material.h"
#include "voxel/Voxel.h"
#include "core/tests/TestHelper.h"
#include "core/tests/TestColorHelper.h"
#include "voxel/tests/VoxelPrinter.h"
#include "math/tests/TestMathHelper.h"
#include <gtest/gtest.h>
#include "math/Vec3f.h"

namespace palette {
class Palette;
}
namespace voxel {
class RawVolume;
}
namespace scenegraph {
class SceneGraph;
}

namespace math {
class Mat4f;
class Mat3f;
class Mat4x3f;
::std::ostream &operator<<(::std::ostream &os, const Mat4f &matrix);
::std::ostream &operator<<(::std::ostream &os, const Mat3f &matrix);
::std::ostream &operator<<(::std::ostream &os, const Mat4x3f &matrix);
}

namespace palette {
::std::ostream &operator<<(::std::ostream &os, const palette::Palette &palette);
::std::ostream &operator<<(::std::ostream &os, const palette::Material &material);
}

namespace voxel {

void partialPaletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, voxel::ValidateFlags flags, float maxDelta);
void paletteComparatorScaled(const palette::Palette &pal1, const palette::Palette &pal2, int maxDelta = 4);
void orderPaletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, float maxDelta = 0.001f);
void paletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, float maxDelta = 0.001f);
void colorComparator(const palette::Palette &pal1, const palette::Palette &pal2, color::RGBA c1, color::RGBA c2, uint8_t palIdx, float maxDelta = 0.001f);
void colorComparator(color::RGBA c1, color::RGBA c2, int maxDelta = 1);
void colorComparatorDistance(color::RGBA c1, color::RGBA c2, float maxDelta = 0.001f);
void volumeComparator(const voxel::RawVolume& volume1, const palette::Palette &pal1, const voxel::RawVolume& volume2, const palette::Palette &pal2, ValidateFlags flags, float maxDelta = 0.001f);
void sceneGraphComparator(const scenegraph::SceneGraph &graph1, const scenegraph::SceneGraph &graph2, ValidateFlags flags, float maxDelta = 0.001f);
bool occupiedWorldAABB(const scenegraph::SceneGraph &sceneGraph, math::Vec3f &mins, math::Vec3f &maxs);
void occupiedWorldComparator(const scenegraph::SceneGraph &src, const scenegraph::SceneGraph &dst, bool dstMayBeSubset,
							 float aabbDelta = 0.01f);

// this also allows you to skip some material properties, because a format might not support it.
// ignoreType skips MaterialType (not expressible in stock glTF).
void materialComparator(const scenegraph::SceneGraph &graph1, const scenegraph::SceneGraph &graph2,
						const core::Buffer<palette::MaterialProperty> &ignoredMaterials = {}, bool ignoreType = false);
void materialComparator(const palette::Palette &pal1, const palette::Palette &pal2,
						const core::Buffer<palette::MaterialProperty> &ignoredMaterials = {}, bool ignoreType = false);

}
