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

inline constexpr const char *ReskinModeStr[] = {NC_("Reskin Modes", "Replace"), NC_("Reskin Modes", "Blend"),
												NC_("Reskin Modes", "Negate")};
static_assert(lengthof(ReskinModeStr) == (int)voxelutil::ReskinMode::Max, "ReskinModeStr size mismatch");

inline constexpr const char *ReskinFollowStr[] = {NC_("Reskin Follow", "None"), NC_("Reskin Follow", "Median"),
												  NC_("Reskin Follow", "Voxel")};
static_assert(lengthof(ReskinFollowStr) == (int)voxelutil::ReskinFollow::Max, "ReskinFollowStr size mismatch");

inline constexpr const char *ReskinAnchorStr[] = {NC_("Reskin Anchor", "Min/Min"), NC_("Reskin Anchor", "Min/Max"),
												  NC_("Reskin Anchor", "Max/Min"), NC_("Reskin Anchor", "Max/Max")};
static_assert(lengthof(ReskinAnchorStr) == (int)voxelutil::ReskinAnchor::Max, "ReskinAnchorStr size mismatch");

inline constexpr const char *ReskinRotationStr[] = {NC_("Reskin Rotation", "0"), NC_("Reskin Rotation", "90"),
													NC_("Reskin Rotation", "180"), NC_("Reskin Rotation", "270")};
static_assert(lengthof(ReskinRotationStr) == (int)voxelutil::ReskinRotation::Max, "ReskinRotationStr size mismatch");

inline constexpr const char *ReskinTileStr[] = {NC_("Reskin Tile", "Once"), NC_("Reskin Tile", "Repeat"),
												NC_("Reskin Tile", "Stretch")};
static_assert(lengthof(ReskinTileStr) == (int)voxelutil::ReskinTile::Max, "ReskinTileStr size mismatch");

inline constexpr const char *ReskinSkinAxisStr[] = {NC_("Reskin Skin Axis", "X"), NC_("Reskin Skin Axis", "Y"),
													NC_("Reskin Skin Axis", "Z")};
static_assert(lengthof(ReskinSkinAxisStr) == 3, "ReskinSkinAxisStr size mismatch");

inline constexpr const char *VoxelSamplingStr[] = {NC_("Scale Sampling", "Nearest"), NC_("Scale Sampling", "Linear"),
												   NC_("Scale Sampling", "Cubic")};
static_assert(lengthof(VoxelSamplingStr) == (int)voxel::VoxelSampling::Max, "VoxelSamplingStr size mismatch");

inline constexpr size_t BrushPanelDeferredTransformThreshold = 10000;

} // namespace voxedit
