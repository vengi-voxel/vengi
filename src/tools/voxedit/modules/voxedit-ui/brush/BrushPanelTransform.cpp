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

void BrushPanelTransform::executeTransformBrush(BrushPanelContext &ctx) {
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

void BrushPanelTransform::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	TransformBrush &brush = modifier.transformBrush();

	const int nodeId = ctx.sceneMgr->sceneGraph().activeNode();
	if (!ctx.sceneMgr->hasSelection(nodeId)) {
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
				[this, &ctx, &brush, i]() {
					if (_dirty || brush.hasSnapshot()) {
						executeTransformBrush(ctx);
						_dirty = false;
					}
					brush.setTransformMode((TransformMode)i);
				},
				!active);
		}
	}

	switch (brush.transformMode()) {
	case TransformMode::Move: {
		_moveOffset = brush.moveOffset();
		if (ImGui::AxisSliders(_moveOffset, -TransformBrush::MaxMoveOffset, TransformBrush::MaxMoveOffset)) {
			brush.setMoveOffset(_moveOffset);
			executeTransformBrush(ctx);
		}
		break;
	}

	case TransformMode::Shear: {
		_shearOffset = brush.shearOffset();
		if (ImGui::AxisSliders(_shearOffset, -TransformBrush::MaxShearOffset, TransformBrush::MaxShearOffset)) {
			brush.setShearOffset(_shearOffset);
			executeTransformBrush(ctx);
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
			const voxel::Region &snapshotRegion = brush.snapshotRegion();
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
					const glm::ivec3 changed = _targetSize - prevTargetSize;
					int changedAxis = -1;
					for (int i = 0; i < 3; ++i) {
						if (changed[i] != 0) {
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
			brush.setScale(_scale);
			if (brush.snapshotVoxelCount() <= BrushPanelDeferredTransformThreshold) {
				executeTransformBrush(ctx);
			} else {
				_dirty = true;
			}
		}
		break;
	}

	case TransformMode::Rotate: {
		_rotation = brush.rotationDegrees();
		if (ImGui::AxisSliders(_rotation, -360.0f, 360.0f, "%.1f")) {
			brush.setRotationDegrees(_rotation);
			if (brush.snapshotVoxelCount() <= BrushPanelDeferredTransformThreshold) {
				executeTransformBrush(ctx);
			} else {
				_dirty = true;
			}
		}
		break;
	}

	default:
		break;
	}

	if (brush.transformMode() == TransformMode::Scale || brush.transformMode() == TransformMode::Rotate) {
		if (_dirty) {
			if (ImGui::ButtonFullWidth(_("Apply"))) {
				executeTransformBrush(ctx);
				_dirty = false;
			}
		}
		int samplingInt = (int)brush.voxelSampling();
		if (ImGui::BeginCombo(_("Sampling"), _(VoxelSamplingStr[samplingInt]))) {
			for (int i = 0; i < (int)voxel::VoxelSampling::Max; ++i) {
				const bool selected = samplingInt == i;
				if (ImGui::Selectable(_(VoxelSamplingStr[i]), selected)) {
					brush.setVoxelSampling((voxel::VoxelSampling)i);
					_dirty = true;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
}

} // namespace voxedit
