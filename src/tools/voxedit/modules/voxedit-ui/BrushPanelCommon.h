/**
 * @file
 */

#pragma once

#include "app/I18NMarkers.h"
#include "core/ArrayLength.h"
#include "ui/IconsLucide.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxel/VoxelSampling.h"
#include "voxelutil/VolumeSculpt.h"

namespace voxedit {

inline constexpr const char *BrushTypeIcons[] = {
	ICON_LC_PIPETTE,  ICON_LC_BOXES,		 ICON_LC_GROUP,		ICON_LC_STAMP,
	ICON_LC_PEN_LINE, ICON_LC_PAINTBRUSH,	 ICON_LC_TEXT_WRAP, ICON_LC_SQUARE_DASHED_MOUSE_POINTER,
	ICON_LC_IMAGE,	  ICON_LC_MOVE_UP_RIGHT, ICON_LC_EXPAND,	ICON_LC_MOVE_3D,
	ICON_LC_BLEND,	  ICON_LC_RULER,		 ICON_LC_SCROLL};
static_assert(lengthof(BrushTypeIcons) == (int)BrushType::Max, "BrushTypeIcons size mismatch");

inline constexpr const char *ReskinModeStr[] = {
	// Skin solid overwrites surface; skin air removes surface voxels
	NC_("Sculpt reskin apply mode", "Replace"),
	// Skin solid overwrites surface; skin air preserves surface voxels
	NC_("Sculpt reskin apply mode", "Blend"),
	// Skin solid removes surface voxels; skin air preserves surface voxels
	NC_("Sculpt reskin apply mode", "Negate")};
static_assert(lengthof(ReskinModeStr) == (int)voxelutil::ReskinMode::Max, "ReskinModeStr size mismatch");

inline constexpr const char *ReskinFollowStr[] = {
	// Flat plane at max/min surface height
	NC_("Sculpt reskin surface follow", "None"),
	// Flat plane at median surface height
	NC_("Sculpt reskin surface follow", "Median"),
	// Per-column surface following
	NC_("Sculpt reskin surface follow", "Voxel"),
	// Bilinear interpolation from 4 corner averages
	NC_("Sculpt reskin surface follow", "Corner Average")};
static_assert(lengthof(ReskinFollowStr) == (int)voxelutil::ReskinFollow::Max, "ReskinFollowStr size mismatch");

inline constexpr const char *ReskinAnchorStr[] = {
	// Skin (0,0) maps to selection UV corner
	NC_("Sculpt reskin UV anchor", "Min/Min"), NC_("Sculpt reskin UV anchor", "Min/Max"),
	NC_("Sculpt reskin UV anchor", "Max/Min"), NC_("Sculpt reskin UV anchor", "Max/Max")};
static_assert(lengthof(ReskinAnchorStr) == (int)voxelutil::ReskinAnchor::Max, "ReskinAnchorStr size mismatch");

inline constexpr const char *ReskinRotationStr[] = {NC_("Sculpt reskin rotation degrees", "0"),
													NC_("Sculpt reskin rotation degrees", "90"),
													NC_("Sculpt reskin rotation degrees", "180"),
													NC_("Sculpt reskin rotation degrees", "270")};
static_assert(lengthof(ReskinRotationStr) == (int)voxelutil::ReskinRotation::Max, "ReskinRotationStr size mismatch");

inline constexpr const char *ReskinTileStr[] = {
	// Apply skin once, stop at skin boundary
	NC_("Sculpt reskin tiling", "Once"),
	// Tile across selection, with optional mirroring
	NC_("Sculpt reskin tiling", "Repeat"),
	// Scale skin to fill selection UV extents
	NC_("Sculpt reskin tiling", "Stretch")};
static_assert(lengthof(ReskinTileStr) == (int)voxelutil::ReskinTile::Max, "ReskinTileStr size mismatch");

inline constexpr const char *ReskinSkinAxisStr[] = {NC_("Sculpt reskin skin axis", "X"),
													NC_("Sculpt reskin skin axis", "Y"),
													NC_("Sculpt reskin skin axis", "Z")};
static_assert(lengthof(ReskinSkinAxisStr) == 3, "ReskinSkinAxisStr size mismatch");

inline constexpr const char *VoxelSamplingStr[] = {
	NC_("Voxel scale sampling", "Nearest"), NC_("Voxel scale sampling", "Linear"),
	NC_("Voxel scale sampling", "Cubic")};
static_assert(lengthof(VoxelSamplingStr) == (int)voxel::VoxelSampling::Max, "VoxelSamplingStr size mismatch");

inline constexpr const char *SmoothWallInterpStr[] = {
	NC_("Sculpt smooth wall interpolation", "Linear"),
	NC_("Sculpt smooth wall interpolation", "Inverse Distance"),
	NC_("Sculpt smooth wall interpolation", "Edge Aware")};
static_assert(lengthof(SmoothWallInterpStr) == (int)voxelutil::SmoothWallInterp::Max, "SmoothWallInterpStr size mismatch");

inline constexpr size_t BrushPanelDeferredTransformThreshold = 10000;

} // namespace voxedit
