/**
 * @file
 */

#include "GameModePanel.h"
#include "MainWindow.h"
#include "command/CommandHandler.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "util/TextProcessor.h"
#include "voxedit-ui/Viewport.h"

namespace voxedit {

void GameModePanel::init() {
	_clipping = core::getVar(cfg::GameModeClipping);
	_applyGravity = core::getVar(cfg::GameModeApplyGravity);
	_movementSpeed = core::getVar(cfg::GameModeMovementSpeed);
	_jumpVelocity = core::getVar(cfg::GameModeJumpVelocity);
	_bodyHeight = core::getVar(cfg::GameModeBodyHeight);
	_gravity = core::getVar(cfg::GameModeGravity);
	_friction = core::getVar(cfg::GameModeFriction);
	_bodySize = core::getVar(cfg::GameModeBodySize);
}

void GameModePanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(NetworkPanel);
	const core::String title = makeTitle(ICON_LC_GAMEPAD, _("Game mode"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		const char *text = _("Activating the game mode will enable clipping and switch the eye mode camera that is "
							 "controlled by <cmd:+move_forward>, <cmd:+move_left>, <cmd:+move_backward>, "
							 "<cmd:+move_right> and <cmd:+jump> for jumping");
		static char buf[4096];
		if (util::replacePlaceholders(_app->keybindingHandler(), text, buf, sizeof(buf))) {
			ImGui::TextWrappedUnformatted(buf);
		}
		bool gameModeEnabled = _gameModeEnabled;
		if (ImGui::IconCheckbox(ICON_LC_GAMEPAD, _("Enable"), &gameModeEnabled)) {
			if (Viewport *viewport = _mainWindow->activeViewport()) {
				_clipping->setVal(gameModeEnabled);
				_applyGravity->setVal(gameModeEnabled);
				if (!_gameModeEnabled && gameModeEnabled) {
					viewport->camera().setRotationType(video::CameraRotationType::Eye);
					viewport->resetCamera();
				}
				_gameModeEnabled = gameModeEnabled;
			}
		}

		ImGui::BeginDisabled(_gameModeEnabled == false);
		ImGui::InputVarFloat(_movementSpeed, 0.1f, 100.0f);
		ImGui::InputVarFloat(_jumpVelocity, 0.1f, 100.0f);
		ImGui::InputVarFloat(_bodyHeight, 0.1f, 10.0f);
		ImGui::InputVarFloat(_gravity, 0.01f, 100.0f);
		ImGui::InputVarFloat(_friction, 0.001f, 1.0f);
		ImGui::InputVarFloat(_bodySize, 0.2f, 0.4998f);

		if (ImGui::Button(_("Minecraft"))) {
			_bodyHeight->setVal(1.8f);
		}
		ImGui::SameLine();
		if (ImGui::Button(_("Ace Of Spades"))) {
			_bodyHeight->setVal(2.8f);
		}

		ImGui::EndDisabled();
	}
	ImGui::End();
}

} // namespace voxedit
