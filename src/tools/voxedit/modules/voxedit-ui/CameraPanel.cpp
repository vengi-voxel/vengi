/**
 * @file
 */

#include "CameraPanel.h"
#include "voxedit-util/SceneManager.h"
#include "ui/IconsLucide.h"
#include "command/CommandHandler.h"
#include "app/I18N.h"
#include "ui/IMGUIEx.h"
#include "video/Camera.h"
#include "voxelrender/SceneGraphRenderer.h"

namespace voxedit {

void CameraPanel::update(const char *id, video::Camera &camera, command::CommandExecutionListener &listener) {
	core_trace_scoped(CameraPanel);
	const core::String title = makeTitle(ICON_LC_CAMERA, _("Camera"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		glm::vec3 pos = camera.worldPosition();
		if (ImGui::InputXYZ(_("Position"), pos)) {
			camera.setWorldPosition(pos);
		}
		glm::vec3 target = camera.target();
		if (ImGui::InputXYZ(_("Target"), target)) {
			camera.setTarget(target);
		}
		float targetDistance = camera.targetDistance();
		if (ImGui::InputFloat(_("Target distance"), &targetDistance)) {
			camera.setTargetDistance(targetDistance);
		}
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
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_SQUARE_PLUS, _("Add new camera"))) {
			scenegraph::SceneGraphNodeCamera cameraNode = voxelrender::toCameraNode(camera);
			_sceneMgr->moveNodeToSceneGraph(cameraNode);
		}
	}
	ImGui::End();
}

} // namespace voxedit
