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

	if (paintMode == PaintBrush::PaintMode::Brighten || paintMode == PaintBrush::PaintMode::Darken ||
		paintMode == PaintBrush::PaintMode::Variation) {
		float factor = brush.factor();
		if (ImGui::InputFloat(_("Strength"), &factor)) {
			brush.setFactor(factor);
		}
		ImGui::TooltipTextUnformatted(_("Amount of brighten or darken to apply"));
	}
	if (paintMode == PaintBrush::PaintMode::Variation) {
		int variationThreshold = brush.variationThreshold();
		if (ImGui::InputInt(_("Variation chance (1 in N)"), &variationThreshold)) {
			brush.setVariationThreshold(variationThreshold);
		}
		ImGui::TooltipTextUnformatted(_("Each voxel has a 1 in N chance to be varied"));
	}

	brushpanel::aabbBrushOptions(listener, brush);
	if (ImGui::RadioButton(_("Flood fill"), brush.plane())) {
		brush.setPlane();
	}
	ImGui::TooltipTextUnformatted(_("Fill connected voxels of the same color on the clicked face"));

	if (ImGui::RadioButton(_("Gradient"), brush.gradient())) {
		brush.setGradient();
	}
	ImGui::TooltipTextUnformatted(_("Blend from the hit color to the cursor color across the box"));

	brushpanel::aabbBrushModeOptions(brush);
}

} // namespace voxedit
