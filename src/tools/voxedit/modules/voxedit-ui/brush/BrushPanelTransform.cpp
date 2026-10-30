/**
 * @file
 */

#include "BrushPanelTransform.h"
#include "../BrushPanelCommon.h"
#include "Toolbar.h"
#include "app/I18N.h"
#include "command/CommandHandler.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/TransformBrush.h"
#include "voxel/Region.h"

#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

void BrushPanelTransform::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	TransformBrush &brush = modifier.transformBrush();
	TransformParameters parameters = brush.parameters();
	bool changed = false;

	if (!ctx.sceneMgr->hasSelection(ctx.sceneMgr->activeNodeUUID())) {
		ImGui::TextWrappedUnformatted(_("No selection active - use the Select brush first"));
		return;
	}

	const TransformMode currentTransformMode = brush.transformMode();
	{
		ui::Toolbar toolbar("transformmode");
		for (int i = 0; i < (int)TransformMode::Max; ++i) {
			const bool active = (TransformMode)i == currentTransformMode;
			toolbar.button(
				TransformModeIcons[i], _(TransformModeStr[i]),
				[&modifier, &brush, i]() {
					TransformParameters next;
					next.mode = (TransformMode)i;
					next.sampling = brush.voxelSampling();
					modifier.updateParameters<TransformBrush>("transform", next);
				},
				!active);
		}
	}

	switch (brush.transformMode()) {
	case TransformMode::Move: {
		_moveOffset = brush.moveOffset();
		if (ImGui::AxisSliders(_moveOffset, -TransformBrush::MaxMoveOffset, TransformBrush::MaxMoveOffset)) {
			parameters.translation = _moveOffset;
			changed = true;
		}
		break;
	}

	case TransformMode::Shear: {
		_shearOffset = brush.shearOffset();
		if (ImGui::AxisSliders(_shearOffset, -TransformBrush::MaxShearOffset, TransformBrush::MaxShearOffset)) {
			parameters.shear = _shearOffset;
			changed = true;
		}
		break;
	}

	case TransformMode::Scale: {
		_scale = brush.scale();
		if (_scale == glm::vec3(1.0f)) {
			_targetSize = glm::ivec3(0);
		}
		ImGui::Checkbox(_("Voxel size"), &_useVoxelSize);
		ImGui::SameLine();
		if (_useVoxelSize) {
			ImGui::Checkbox(_("Maintain aspect ratio"), &_maintainAspectRatio);
		} else {
			ImGui::Checkbox(_("Uniform"), &_uniformScale);
		}
		bool scaleChanged = false;
		if (_useVoxelSize) {
			const voxel::Region snapshotRegion = ctx.sceneMgr->selectionCalculateRegion(ctx.sceneMgr->activeNodeUUID());
			const glm::ivec3 originalSize =
				snapshotRegion.isValid() ? snapshotRegion.getDimensionsInVoxels() : glm::ivec3(1);
			if (_targetSize.x <= 0 || _targetSize.y <= 0 || _targetSize.z <= 0) {
				_targetSize = originalSize;
			}
			static constexpr int MinTargetSize = 1;
			static constexpr int MaxTargetSize = 4096;
			const glm::ivec3 prevTargetSize = _targetSize;
			if (ImGui::AxisSliders(_targetSize, MinTargetSize, MaxTargetSize)) {
				if (_maintainAspectRatio && originalSize.x > 0 && originalSize.y > 0 && originalSize.z > 0) {
					const glm::ivec3 delta = _targetSize - prevTargetSize;
					int changedAxis = -1;
					for (int i = 0; i < 3; ++i) {
						if (delta[i] != 0) {
							changedAxis = i;
							break;
						}
					}
					if (changedAxis >= 0) {
						const float ratio = (float)_targetSize[changedAxis] / (float)originalSize[changedAxis];
						for (int i = 0; i < 3; ++i) {
							if (i != changedAxis) {
								_targetSize[i] =
									glm::max((int)glm::round((float)originalSize[i] * ratio), MinTargetSize);
							}
						}
					}
				}
				_scale = glm::vec3(_targetSize) / glm::vec3(originalSize);
				_scale = glm::max(_scale, glm::vec3(0.01f));
				scaleChanged = true;
			}
			ImGui::TextDisabled(_("Original: %i x %i x %i"), originalSize.x, originalSize.y, originalSize.z);
		} else {
			if (_uniformScale) {
				float uniform = _scale.x;
				ImGui::SetNextItemWidth(-1);
				if (ImGui::SliderFloat("##uniform_scale", &uniform, 0.01f, 4.0f, "%.2f")) {
					_scale = glm::vec3(uniform);
					scaleChanged = true;
				}
			} else {
				scaleChanged = ImGui::AxisSliders(_scale, 0.01f, 4.0f, "%.2f");
			}
		}
		if (scaleChanged) {
			parameters.scale = _scale;
			changed = true;
		}
		break;
	}

	case TransformMode::Rotate: {
		_rotation = brush.rotationDegrees();
		if (ImGui::AxisSliders(_rotation, -360.0f, 360.0f, "%.1f")) {
			parameters.rotationDegrees = _rotation;
			changed = true;
		}
		break;
	}

	default:
		break;
	}

	if (brush.transformMode() == TransformMode::Scale || brush.transformMode() == TransformMode::Rotate) {

		int samplingInt = (int)brush.voxelSampling();
		if (ImGui::BeginCombo(_("Sampling"), _(VoxelSamplingStr[samplingInt]))) {
			for (int i = 0; i < (int)voxel::VoxelSampling::Max; ++i) {
				const bool selected = samplingInt == i;
				if (ImGui::Selectable(_(VoxelSamplingStr[i]), selected)) {
					parameters.sampling = (voxel::VoxelSampling)i;
					changed = true;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
	if (changed) {
		modifier.updateParameters<TransformBrush>("transform", parameters);
	}
}

} // namespace voxedit
