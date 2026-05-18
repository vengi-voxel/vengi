/**
 * @file
 */

#include "BrushPanelSculpt.h"
#include "../BrushPanelCommon.h"
#include "Style.h"
#include "app/I18N.h"
#include "command/CommandHandler.h"
#include "core/Log.h"
#include "io/FilesystemArchive.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/SculptBrush.h"
#include "voxel/ClipboardData.h"
#include "voxel/Face.h"
#include "voxelformat/VolumeFormat.h"

#include <glm/trigonometric.hpp>

namespace voxedit {

void BrushPanelSculpt::executeSculptBrush(BrushPanelContext &ctx) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	if (!modifier.beginBrushFromPanel()) {
		return;
	}
	auto func = [&](int nodeId) {
		if (scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNode(nodeId)) {
			if (!node->visible()) {
				return;
			}
			auto callback = [&](const voxel::Region &region, ModifierType type, SceneModifiedFlags flags) {
				ctx.sceneMgr->modified(nodeId, region, flags);
			};
			modifier.execute(ctx.sceneMgr->sceneGraph(), *node, callback);
		}
	};
	ctx.sceneMgr->nodeForeachGroup(func);
	modifier.endBrush();
}

void BrushPanelSculpt::loadSkinFromFile(BrushPanelContext &ctx, const core::String &filename) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(ctx.app->filesystem());
	scenegraph::SceneGraph sceneGraph;
	voxelformat::LoadContext loadCtx;
	io::FileDescription fileDesc;
	fileDesc.set(filename);
	if (!voxelformat::loadFormat(fileDesc, archive, sceneGraph, loadCtx)) {
		Log::error("Failed to load skin file: %s", filename.c_str());
		return;
	}

	scenegraph::SceneGraph::MergeResult merged = sceneGraph.merge();
	// volume() transfers ownership (nullifies internal pointer)
	voxel::RawVolume *volume = merged.volume();
	if (volume == nullptr) {
		Log::error("No model node found in skin file: %s", filename.c_str());
		return;
	}

	Modifier &modifier = ctx.sceneMgr->modifier();
	SculptBrush &brush = modifier.sculptBrush();
	brush.setOwnedSkinVolume(volume, filename);
	executeSculptBrush(ctx);
}

void BrushPanelSculpt::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	SculptBrush &brush = modifier.sculptBrush();

	const int nodeId = ctx.sceneMgr->sceneGraph().activeNode();
	if (!brush.hasSnapshot() && !ctx.sceneMgr->hasSelection(nodeId)) {
		ImGui::TextWrappedUnformatted(_("Select voxels first, then switch to sculpt"));
		return;
	}

	const SculptMode currentMode = brush.sculptMode();

	// Keep skin volume up to date for Reskin mode so it's ready when the user clicks a face.
	// Only recompute the color remap when the clipboard volume pointer changes.
	if (currentMode == SculptMode::Reskin) {
		const voxel::ClipboardData &clip = ctx.sceneMgr->clipboardData();
		if (clip && clip.volume != nullptr && clip.volume != brush.skinVolume()) {
			brush.setSkinVolume(clip.volume);
		}
	}

	const core::String currentSculptLabel =
		core::String::format("%s %s", SculptModeIcons[(int)currentMode], _(SculptModeStr[(int)currentMode]));
	if (ImGui::BeginCombo(_("Sculpt mode"), currentSculptLabel.c_str(), ImGuiComboFlags_None)) {
		for (int i = 0; i < (int)SculptMode::Max; ++i) {
			const SculptMode mode = (SculptMode)i;
			const bool selected = mode == currentMode;
			const core::String sculptLabel = core::String::format("%s %s", SculptModeIcons[i], _(SculptModeStr[i]));
			if (ImGui::Selectable(sculptLabel.c_str(), selected)) {
				brush.setSculptMode(mode);
				executeSculptBrush(ctx);
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	const bool needsFace = SculptBrush::modeNeedsFace(currentMode);
	if (needsFace) {
		const voxel::FaceNames flattenFace = brush.flattenFace();
		if (flattenFace == voxel::FaceNames::Max) {
			const glm::vec4 &warningTextColor = style::color(style::StyleColor::ColorWarningText);
			ImGui::TextColored(warningTextColor, "%s", _("Click a voxel face in the viewport to set the direction"));
			return;
		}
		ImGui::Text(_("Direction: %s"), voxel::faceNameString(flattenFace));
	}

	if (currentMode == SculptMode::SmoothAdditive) {
		int threshold = brush.heightThreshold();
		if (ImGui::SliderInt(_("Height threshold"), &threshold, 1, SculptBrush::MaxHeightThreshold)) {
			brush.setHeightThreshold(threshold);
			executeSculptBrush(ctx);
		}
	} else if (currentMode == SculptMode::SmoothErode) {
		bool preserve = brush.preserveTopHeight();
		if (ImGui::Checkbox(_("Preserve top height"), &preserve)) {
			brush.setPreserveTopHeight(preserve);
			executeSculptBrush(ctx);
		}
		if (preserve) {
			int trimPerStep = brush.trimPerStep();
			ImGui::TextUnformatted(_("Trim per step"));
			if (ImGui::Button("-##sculpt_trim")) {
				brush.setTrimPerStep(trimPerStep - 1);
				executeSculptBrush(ctx);
			}
			ImGui::SameLine();
			if (ImGui::SliderInt("##sculpt_trim_slider", &trimPerStep, 1, SculptBrush::MaxTrimPerStep)) {
				brush.setTrimPerStep(trimPerStep);
				executeSculptBrush(ctx);
			}
			ImGui::SameLine();
			if (ImGui::Button("+##sculpt_trim")) {
				brush.setTrimPerStep(trimPerStep + 1);
				executeSculptBrush(ctx);
			}
		}
	} else if (currentMode == SculptMode::SmoothGaussian) {
		int kernelSize = brush.kernelSize();
		ImGui::TextUnformatted(_("Kernel size"));
		if (ImGui::Button("-##sculpt_kernel")) {
			brush.setKernelSize(kernelSize - 1);
			executeSculptBrush(ctx);
		}
		ImGui::SameLine();
		if (ImGui::SliderInt("##sculpt_kernel_slider", &kernelSize, 1, SculptBrush::MaxKernelSize)) {
			brush.setKernelSize(kernelSize);
			executeSculptBrush(ctx);
		}
		ImGui::SameLine();
		if (ImGui::Button("+##sculpt_kernel")) {
			brush.setKernelSize(kernelSize + 1);
			executeSculptBrush(ctx);
		}
		float sigma = brush.sigma();
		if (ImGui::SliderFloat(_("Sigma"), &sigma, SculptBrush::MinSigma, SculptBrush::MaxSigma)) {
			brush.setSigma(sigma);
			executeSculptBrush(ctx);
		}
	} else if (currentMode == SculptMode::Reskin) {
		if (brush.skinVolume() == nullptr) {
			const glm::vec4 &warningTextColor = style::color(style::StyleColor::ColorWarningText);
			ImGui::TextColored(warningTextColor, "%s", _("Copy voxels to clipboard or load a skin file"));
		}

		// Load skin from file / Reload buttons
		if (ImGui::Button(_("Load skin"))) {
			ctx.app->openDialog([this, &ctx](const core::String &filename,
											 const io::FormatDescription *) { loadSkinFromFile(ctx, filename); },
								{}, voxelformat::voxelLoad());
		}
		const core::String &skinPath = brush.skinFilePath();
		if (!skinPath.empty()) {
			ImGui::SameLine();
			if (ImGui::Button(_("Reload"))) {
				loadSkinFromFile(ctx, skinPath);
			}
			ImGui::SetItemTooltipUnformatted(skinPath.c_str());
		}

		const voxelutil::ReskinConfig &cfg = brush.reskinConfig();

		// Skin up axis combo (which axis of the skin is outward)
		{
			static constexpr math::Axis skinAxisValues[] = {math::Axis::X, math::Axis::Y, math::Axis::Z};
			const int currentAxisIdx = math::getIndexForAxis(cfg.skinUpAxis);
			if (ImGui::BeginCombo(_("Skin up axis"), _(ReskinSkinAxisStr[currentAxisIdx]), ImGuiComboFlags_None)) {
				for (int i = 0; i < 3; ++i) {
					const bool selected = i == currentAxisIdx;
					if (ImGui::Selectable(_(ReskinSkinAxisStr[i]), selected)) {
						brush.setReskinSkinUpAxis(skinAxisValues[i]);
						executeSculptBrush(ctx);
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}
		ImGui::SetItemTooltipUnformatted(_("Which axis of the skin points outward from the surface. "
										   "Set to Y if your skin was built with the pattern facing up."));

		// Reskin mode combo
		if (ImGui::BeginCombo(_("Reskin mode"), _(ReskinModeStr[(int)cfg.mode]), ImGuiComboFlags_None)) {
			for (int i = 0; i < (int)voxelutil::ReskinMode::Max; ++i) {
				const bool selected = i == (int)cfg.mode;
				if (ImGui::Selectable(_(ReskinModeStr[i]), selected)) {
					brush.setReskinMode((voxelutil::ReskinMode)i);
					executeSculptBrush(ctx);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		// Follow mode combo
		if (ImGui::BeginCombo(_("Follow surface"), _(ReskinFollowStr[(int)cfg.follow]), ImGuiComboFlags_None)) {
			for (int i = 0; i < (int)voxelutil::ReskinFollow::Max; ++i) {
				const bool selected = i == (int)cfg.follow;
				if (ImGui::Selectable(_(ReskinFollowStr[i]), selected)) {
					brush.setReskinFollow((voxelutil::ReskinFollow)i);
					executeSculptBrush(ctx);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		// Anchor combo
		if (ImGui::BeginCombo(_("Anchor"), _(ReskinAnchorStr[(int)cfg.anchor]), ImGuiComboFlags_None)) {
			for (int i = 0; i < (int)voxelutil::ReskinAnchor::Max; ++i) {
				const bool selected = i == (int)cfg.anchor;
				if (ImGui::Selectable(_(ReskinAnchorStr[i]), selected)) {
					brush.setReskinAnchor((voxelutil::ReskinAnchor)i);
					executeSculptBrush(ctx);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		// Rotation combo
		if (ImGui::BeginCombo(_("Rotation"), _(ReskinRotationStr[(int)cfg.rotation]), ImGuiComboFlags_None)) {
			for (int i = 0; i < (int)voxelutil::ReskinRotation::Max; ++i) {
				const bool selected = i == (int)cfg.rotation;
				if (ImGui::Selectable(_(ReskinRotationStr[i]), selected)) {
					brush.setReskinRotation((voxelutil::ReskinRotation)i);
					executeSculptBrush(ctx);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		// Tile mode combo
		if (ImGui::BeginCombo(_("Tile"), _(ReskinTileStr[(int)cfg.tile]), ImGuiComboFlags_None)) {
			for (int i = 0; i < (int)voxelutil::ReskinTile::Max; ++i) {
				const bool selected = i == (int)cfg.tile;
				if (ImGui::Selectable(_(ReskinTileStr[i]), selected)) {
					brush.setReskinTile((voxelutil::ReskinTile)i);
					executeSculptBrush(ctx);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		// Mirror checkboxes (only for Repeat tile mode)
		if (cfg.tile == voxelutil::ReskinTile::Repeat) {
			bool mirrorU = cfg.mirrorU;
			if (ImGui::Checkbox(_("Mirror U"), &mirrorU)) {
				brush.setReskinMirrorU(mirrorU);
				executeSculptBrush(ctx);
			}
			ImGui::SameLine();
			bool mirrorV = cfg.mirrorV;
			if (ImGui::Checkbox(_("Mirror V"), &mirrorV)) {
				brush.setReskinMirrorV(mirrorV);
				executeSculptBrush(ctx);
			}
		}

		// Offset sliders (not for Stretch)
		if (cfg.tile != voxelutil::ReskinTile::Stretch) {
			int offsetU = cfg.offsetU;
			ImGui::TextUnformatted(_("Offset U"));
			if (ImGui::Button("-##reskin_ou")) {
				brush.setReskinOffsetU(offsetU - 1);
				executeSculptBrush(ctx);
			}
			ImGui::SameLine();
			if (ImGui::SliderInt("##reskin_ou_slider", &offsetU, -32, 32)) {
				brush.setReskinOffsetU(offsetU);
				executeSculptBrush(ctx);
			}
			ImGui::SameLine();
			if (ImGui::Button("+##reskin_ou")) {
				brush.setReskinOffsetU(offsetU + 1);
				executeSculptBrush(ctx);
			}

			int offsetV = cfg.offsetV;
			ImGui::TextUnformatted(_("Offset V"));
			if (ImGui::Button("-##reskin_ov")) {
				brush.setReskinOffsetV(offsetV - 1);
				executeSculptBrush(ctx);
			}
			ImGui::SameLine();
			if (ImGui::SliderInt("##reskin_ov_slider", &offsetV, -32, 32)) {
				brush.setReskinOffsetV(offsetV);
				executeSculptBrush(ctx);
			}
			ImGui::SameLine();
			if (ImGui::Button("+##reskin_ov")) {
				brush.setReskinOffsetV(offsetV + 1);
				executeSculptBrush(ctx);
			}
		}

		// Surface offset and skin depth
		int surfaceOffset = cfg.zOffset;
		ImGui::TextUnformatted(_("Surface offset"));
		if (ImGui::Button("-##reskin_so")) {
			brush.setReskinZOffset(surfaceOffset - 1);
			executeSculptBrush(ctx);
		}
		ImGui::SameLine();
		if (ImGui::SliderInt("##reskin_so_slider", &surfaceOffset, -SculptBrush::MaxReskinDepth,
							 SculptBrush::MaxReskinDepth)) {
			brush.setReskinZOffset(surfaceOffset);
			executeSculptBrush(ctx);
		}
		ImGui::SameLine();
		if (ImGui::Button("+##reskin_so")) {
			brush.setReskinZOffset(surfaceOffset + 1);
			executeSculptBrush(ctx);
		}
		ImGui::SetItemTooltipUnformatted(_("Positive = skin floats above surface, negative = sinks below"));

		int skinDepth = cfg.skinDepth;
		ImGui::TextUnformatted(_("Skin layers"));
		if (ImGui::Button("-##reskin_sd")) {
			brush.setReskinSkinDepth(skinDepth - 1);
			executeSculptBrush(ctx);
		}
		ImGui::SameLine();
		if (ImGui::SliderInt("##reskin_sd_slider", &skinDepth, 1, SculptBrush::MaxReskinDepth)) {
			brush.setReskinSkinDepth(skinDepth);
			executeSculptBrush(ctx);
		}
		ImGui::SameLine();
		if (ImGui::Button("+##reskin_sd")) {
			brush.setReskinSkinDepth(skinDepth + 1);
			executeSculptBrush(ctx);
		}

		// Invert checkbox
		bool invertSkin = cfg.invertSkin;
		if (ImGui::Checkbox(_("Invert skin"), &invertSkin)) {
			brush.setReskinInvertSkin(invertSkin);
			executeSculptBrush(ctx);
		}
	} else if (currentMode == SculptMode::ExtendPlane) {
		int radius = brush.brushRadius();
		ImGui::TextUnformatted(_("Brush radius"));
		if (ImGui::Button("-##extend_radius")) {
			brush.setBrushRadius(radius - 1);
		}
		ImGui::SameLine();
		if (ImGui::SliderInt("##extend_radius_slider", &radius, 1, SculptBrush::MaxBrushRadius)) {
			brush.setBrushRadius(radius);
		}
		ImGui::SameLine();
		if (ImGui::Button("+##extend_radius")) {
			brush.setBrushRadius(radius + 1);
		}
		bool extendOnly = brush.extendOnly();
		if (ImGui::Checkbox(_("Extend only"), &extendOnly)) {
			brush.setExtendOnly(extendOnly);
		}
		int removeDepth = brush.removeAboveDepth();
		ImGui::TextUnformatted(_("Remove above"));
		if (ImGui::Button("-##remove_depth")) {
			brush.setRemoveAboveDepth(removeDepth - 1);
		}
		ImGui::SameLine();
		if (ImGui::SliderInt("##remove_depth_slider", &removeDepth, 0, SculptBrush::MaxRemoveAboveDepth)) {
			brush.setRemoveAboveDepth(removeDepth);
		}
		ImGui::SameLine();
		if (ImGui::Button("+##remove_depth")) {
			brush.setRemoveAboveDepth(removeDepth + 1);
		}
		ImGui::TooltipTextUnformatted(_("Perpendicular depth of voxels to remove above the plane (0 = no removal)"));
	} else if (currentMode != SculptMode::Flatten && currentMode != SculptMode::BridgeGap &&
			   currentMode != SculptMode::SquashToPlane) {
		float strength = brush.strength();
		if (ImGui::SliderFloat(_("Strength"), &strength, 0.0f, 1.0f)) {
			brush.setStrength(strength);
			executeSculptBrush(ctx);
		}
	}

	if (currentMode != SculptMode::BridgeGap && currentMode != SculptMode::SquashToPlane &&
		currentMode != SculptMode::Reskin && currentMode != SculptMode::ExtendPlane) {
		const int maxIter = needsFace ? SculptBrush::MaxFlattenIterations : SculptBrush::MaxIterations;
		int iterations = brush.iterations();
		if (needsFace) {
			ImGui::TextUnformatted(_("Iterations"));
			if (ImGui::Button("-##sculpt_iter")) {
				brush.setIterations(iterations - 1);
				executeSculptBrush(ctx);
			}
			ImGui::SameLine();
			if (ImGui::SliderInt("##sculpt_iter_slider", &iterations, 1, maxIter)) {
				brush.setIterations(iterations);
				executeSculptBrush(ctx);
			}
			ImGui::SameLine();
			if (ImGui::Button("+##sculpt_iter")) {
				brush.setIterations(iterations + 1);
				executeSculptBrush(ctx);
			}
		} else {
			if (ImGui::SliderInt(_("Iterations"), &iterations, 1, maxIter)) {
				brush.setIterations(iterations);
				executeSculptBrush(ctx);
			}
		}
	}
}

} // namespace voxedit
