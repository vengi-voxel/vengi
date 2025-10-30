/**
 * @file
 */

#include "GameModePanel.h"
#include "MainWindow.h"
#include "command/CommandHandler.h"
#include "imgui.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "util/TextProcessor.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void GameModePanel::init() {
	_clipping = core::Var::getSafe(cfg::VoxEditClipping);
	_applyGravity = core::Var::getSafe(cfg::VoxEditApplyGravity);
	_movementSpeed = core::Var::getSafe(cfg::VoxEditMovementSpeed);
	_jumpVelocity = core::Var::getSafe(cfg::VoxEditJumpVelocity);
	_bodyHeight = core::Var::getSafe(cfg::VoxEditBodyHeight);
}

void GameModePanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(NetworkPanel);
	const core::String title = makeTitle(ICON_LC_GAMEPAD, _("Game mode"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		const char *text = _("Activating the game mode will enable clipping and switch the eye mode camera that is "
							 "controlled by <cmd:+move_forward>, <cmd:+move_left>, <cmd:+move_right>, "
							 "<cmd:+move_backward> and <cmd:+jump> for jumping");
		static char buf[4096];
		if (util::replacePlaceholders(_app->keybindingHandler(), text, buf, sizeof(buf))) {
			ImGui::TextWrappedUnformatted(buf);
		}
		bool gameModeEnabled = _gameModeEnabled;
		if (ImGui::IconCheckbox(ICON_LC_GAMEPAD, _("Enable"), &gameModeEnabled)) {
			if (Viewport *viewport = _mainWindow->activeViewport()) {
				_clipping->setVal(gameModeEnabled);
				_applyGravity->setVal(gameModeEnabled);
				viewport->camera().setRotationType(video::CameraRotationType::Eye);
				if (!_gameModeEnabled && gameModeEnabled) {
					command::executeCommands("resetcamera", &listener);
				}
				_gameModeEnabled = gameModeEnabled;
			}
		}

		ImGui::BeginDisabled(_gameModeEnabled == false);
		ImGui::InputVarFloat(_("Movement Speed"), _movementSpeed, 0.1f, 100.0f);
		ImGui::InputVarFloat(_("Jump Velocity"), _jumpVelocity, 0.1f, 100.0f);
		ImGui::InputVarFloat(_("Body Height"), _bodyHeight, 0.1f, 10.0f);
		// TODO: make gravity configurable - see CameraMovement
		ImGui::EndDisabled();

	}
	ImGui::End();
}

} // namespace voxedit
