/**
 * @file
 */

#include "CameraPanel.h"
#include "command/CommandHandler.h"
#include "ui/IMGUIEx.h"
#include "video/Camera.h"

namespace voxedit {

void CameraPanel::update(const char *title, video::Camera &camera, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		glm::vec3 pos = camera.worldPosition();
		ImGui::InputFloat3(_("Position"), &pos.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
		float farplane = camera.farPlane();
		if (ImGui::InputFloat(_("Farplane"), &farplane, 0.0f, 0.0f, "%.3f")) {
			camera.setFarPlane(farplane);
		}
		float nearplane = camera.nearPlane();
		if (ImGui::InputFloat(_("Nearplane"), &nearplane, 0.0f, 0.0f, "%.3f")) {
			camera.setNearPlane(nearplane);
		}
		float fov = camera.fieldOfView();
		if (ImGui::InputFloat(_("FOV"), &fov, 0.0f, 0.0f, "%.3f")) {
			camera.setFieldOfView(fov);
		}
		float aspect = camera.aspect();
		ImGui::InputFloat(_("Aspect"), &aspect, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::Separator();
		if (ImGui::Button(_("Reset"))) {
			command::executeCommands("resetcamera", &listener);
		}
	}
	ImGui::End();
}

} // namespace voxedit
