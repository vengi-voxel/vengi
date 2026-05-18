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
		if (ImGui::InputFloat(_("Factor"), &factor)) {
			brush.setFactor(factor);
		}
	}
	if (paintMode == PaintBrush::PaintMode::Variation) {
		int variationThreshold = brush.variationThreshold();
		if (ImGui::InputInt(_("Variation threshold"), &variationThreshold)) {
			brush.setVariationThreshold(variationThreshold);
		}
	}

	brushpanel::aabbBrushOptions(listener, brush);
	if (ImGui::RadioButton(_("Plane"), brush.plane())) {
		brush.setPlane();
	}
	ImGui::TooltipTextUnformatted(_("Paint the selected plane"));

	if (ImGui::RadioButton(_("Gradient"), brush.gradient())) {
		brush.setGradient();
	}

	brushpanel::aabbBrushModeOptions(brush);
}

} // namespace voxedit
