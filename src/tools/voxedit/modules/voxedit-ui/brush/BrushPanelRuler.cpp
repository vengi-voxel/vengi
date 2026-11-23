/**
 * @file
 */

#include "BrushPanelRuler.h"
#include "app/I18N.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/RulerBrush.h"
#include "math/Vec3i.h"

namespace voxedit {

void BrushPanelRuler::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	RulerBrush &brush = modifier.rulerBrush();
	bool useRefPos = brush.useReferencePos();
	if (ImGui::Checkbox(_("Use reference position"), &useRefPos)) {
		brush.setUseReferencePos(useRefPos);
	}
	ImGui::TooltipTextUnformatted(_("Measure the distance from the reference position to the cursor"));
	if (!useRefPos) {
		ImGui::TextWrappedUnformatted(_("Click two points to measure the distance between them"));
	}
	if (brush.hasMeasurement()) {
		const math::Vec3i &start = brush.startPos();
		const math::Vec3i &end = brush.endPos();
		const math::Vec3i d = brush.delta();
		ImGui::SeparatorText(_("Measurement"));
		ImGui::InputVec3(_("Start"), start);
		ImGui::InputVec3(_("End"), end);
		ImGui::Separator();
		ImGui::InputVec3(_("Delta"), d);
		ImGui::Separator();
		ImGui::Text(_("Length: %.2f"), brush.euclideanDistance());
		ImGui::Text(_("Manhattan: %i"), brush.manhattanDistance());
		ImGui::TooltipTextUnformatted(_("Grid distance: sum of steps along each axis"));
	}
}

} // namespace voxedit
