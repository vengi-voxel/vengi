/**
 * @file
 */

#include "BrushPanelPaint.h"
#include "BrushPanelWidgets.h"
#include "Toolbar.h"
#include "app/I18N.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/PaintBrush.h"

namespace voxedit {

void BrushPanelPaint::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	PaintBrush &brush = modifier.paintBrush();

	PaintBrush::PaintMode paintMode = brush.paintMode();
	{
		ui::Toolbar toolbar("paintmode");
		for (int i = 0; i < (int)PaintBrush::PaintMode::Max; ++i) {
			const bool active = (PaintBrush::PaintMode)i == paintMode;
			toolbar.button(
				PaintBrush::PaintModeIcons[i], _(PaintBrush::PaintModeStr[i]),
				[&brush, i]() { brush.setPaintMode((PaintBrush::PaintMode)i); }, !active);
		}
	}

	if (paintMode == PaintBrush::PaintMode::Blend || paintMode == PaintBrush::PaintMode::Blur) {
		float opacity = brush.blendOpacity();
		if (ImGui::InputFloat(_("Strength"), &opacity)) {
			brush.setBlendOpacity(opacity);
		}
		if (paintMode == PaintBrush::PaintMode::Blur) {
			ImGui::TooltipTextUnformatted(_("How strongly to apply the gaussian blur of neighboring colors (0 = none, 1 = full)"));
		} else {
			ImGui::TooltipTextUnformatted(_("Opacity at the brush center (0 = none, 1 = full)"));
		}
	} else if (paintMode == PaintBrush::PaintMode::Brighten || paintMode == PaintBrush::PaintMode::Darken ||
			   paintMode == PaintBrush::PaintMode::Variation) {
		float strength = brush.strength();
		if (ImGui::InputFloat(_("Strength"), &strength)) {
			brush.setStrength(strength);
		}
		ImGui::TooltipTextUnformatted(_("Amount of brighten or darken to apply"));
	}
	if (paintMode == PaintBrush::PaintMode::Variation) {
		int variationChance = brush.variationChance();
		if (ImGui::InputInt(_("Variation chance (1 in N)"), &variationChance)) {
			brush.setVariationChance(variationChance);
		}
		ImGui::TooltipTextUnformatted(_("Each voxel has a 1 in N chance to be varied"));
	}

	brushpanel::aabbBrushOptions(listener, brush);

	const bool softMode = paintMode == PaintBrush::PaintMode::Blend || paintMode == PaintBrush::PaintMode::Blur;
	if (!softMode) {
		if (ImGui::RadioButton(_("Flood fill"), brush.floodFill())) {
			brush.setFloodFill();
		}
		ImGui::TooltipTextUnformatted(_("Fill connected voxels of the same color on the clicked face"));

		if (ImGui::RadioButton(_("Gradient"), brush.gradient())) {
			brush.setGradient();
		}
		ImGui::TooltipTextUnformatted(_("Blend from the hit color to the cursor color across the box"));
	}

	brushpanel::aabbBrushModeOptions(brush);
}

} // namespace voxedit
