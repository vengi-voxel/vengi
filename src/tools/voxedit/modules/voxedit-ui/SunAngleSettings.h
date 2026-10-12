/**
 * @file
 */
#pragma once

#include "core/Var.h"
#include "ui/IMGUIEx.h"
#include "ui/ScopedID.h"
#include "ui/ScopedStyle.h"
#include <glm/vec3.hpp>

namespace voxedit {

/** @brief Edit scene sun angles while preserving shadow-mode availability. */
inline bool sunAngleSettings(const core::VarPtr &sunAngleVar, bool shadowsEnabled) {
	ui::ScopedStyle style;
	// Sun angle widget - only enabled when shadows are on
	if (!shadowsEnabled) {
		style.setAlpha(ImGui::GetStyle().Alpha * 0.5f);
	}

	ImGui::TextUnformatted(_("Sun angle"));
	if (shadowsEnabled) {
		ImGui::SetItemTooltipUnformatted(_("Controls the direction of the sun for shadow casting"));
	} else {
		ImGui::SetItemTooltipUnformatted(_("Sun angle is only used in Shadows mode"));
	}

	glm::vec3 sunAngle;
	sunAngleVar->vec3Val(&sunAngle[0]);

	bool sunChanged = false;
	ui::ScopedID id("sunangle");
	if (ImGui::SliderFloat(_("Elevation"), &sunAngle.x, -90.0f, 90.0f, "%.1f\xc2\xb0")) {
		sunChanged = shadowsEnabled;
	}
	ImGui::SetItemTooltipUnformatted(_("Sun elevation angle (pitch): -90 (below) to +90 (above)"));

	if (ImGui::SliderFloat(_("Azimuth"), &sunAngle.y, 0.0f, 360.0f, "%.1f\xc2\xb0")) {
		sunChanged = shadowsEnabled;
	}
	ImGui::SetItemTooltipUnformatted(_("Sun azimuth angle (yaw): 0 (North) to 360"));

	ImGui::BeginDisabled(!shadowsEnabled);
	if (ImGui::Button(_("Preset: Noon"))) {
		sunAngle = glm::vec3(60.0f, 135.0f, 0.0f);
		sunChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button(_("Preset: Evening"))) {
		sunAngle = glm::vec3(15.0f, 225.0f, 0.0f);
		sunChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button(_("Preset: Morning"))) {
		sunAngle = glm::vec3(15.0f, 45.0f, 0.0f);
		sunChanged = true;
	}
	ImGui::EndDisabled();

	if (sunChanged) {
		const core::String val = core::String::format("%.2f %.2f %.2f", sunAngle.x, sunAngle.y, sunAngle.z);
		sunAngleVar->setVal(val);
	}
	return sunChanged;
}

} // namespace voxedit
