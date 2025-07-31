/**
 * @file
 */

#include "CameraPanel.h"
#include "ScopedStyle.h"
#include "Toolbar.h"
#include "app/I18N.h"
#include "command/CommandHandler.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "video/Camera.h"
#include "voxedit-util/SceneManager.h"
#include "voxelrender/SceneGraphRenderer.h"

namespace voxedit {

void CameraPanel::addToolbar(command::CommandExecutionListener &listener, video::Camera &camera) {
	ui::Toolbar toolbar("toolbar", &listener);
	toolbar.button(ICON_LC_X, "resetcamera");
	toolbar.button(ICON_LC_SQUARE_PLUS, _("Add new camera"), [&]() {
		scenegraph::SceneGraphNodeCamera cameraNode = voxelrender::toCameraNode(camera);
		_sceneMgr->moveNodeToSceneGraph(cameraNode);
	});
	toolbar.button(ICON_LC_EYE, "cam_activate", _sceneMgr->activeCameraNode() == nullptr);
}

void CameraPanel::update(const char *id, video::Camera &camera, command::CommandExecutionListener &listener) {
	core_trace_scoped(CameraPanel);
	const core::String title = makeTitle(ICON_LC_CAMERA, _("Camera"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		addToolbar(listener, camera);
		if (ImGui::BeginTable("##camera_props", 2, ImGuiTableFlags_SizingStretchProp)) {
			glm::vec3 pos = camera.worldPosition();
			if (ImGui::InputXYZ(_("Position"), pos)) {
				camera.setWorldPosition(pos);
			}
			glm::vec3 target = camera.target();
			if (ImGui::InputXYZ(_("Target"), target)) {
				camera.setTarget(target);
			}
			float targetDistance = camera.targetDistance();
			if (ImGui::InputFloat(_("Target distance"), targetDistance)) {
				camera.setTargetDistance(targetDistance);
			}
			float farplane = camera.farPlane();
			if (ImGui::InputFloat(_("Farplane"), farplane)) {
				camera.setFarPlane(farplane);
			}
			float nearplane = camera.nearPlane();
			if (ImGui::InputFloat(_("Nearplane"), nearplane)) {
				camera.setNearPlane(nearplane);
			}
			float fov = camera.fieldOfView();
			if (ImGui::InputFloat(_("FOV"), fov)) {
				camera.setFieldOfView(fov);
			}
			ImGui::TooltipTextUnformatted(_("Field of view in degrees"));

			float aspect = camera.aspect();
			ImGui::InputFloat(_("Aspect ratio"), aspect, "%0.3f", ImGuiInputTextFlags_ReadOnly);
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

} // namespace voxedit
