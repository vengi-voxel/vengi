/**
 * @file
 */

#include "ToolsPanel.h"
#include "Toolbar.h"
#include "Util.h"
#include "command/CommandHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-ui/Gizmo.h"
#include "voxedit-ui/MainWindow.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

/**
 * @brief This is an interceptor that doesn't allow to run any command or to modify a model reference
 *
 * It instead opens a popup about doing the unreferencing of the model to be able to execute further modifications
 * on the model itself.
 */
struct ReferenceNodeCommandInterceptor : public command::CommandExecutionListener {
	scenegraph::SceneGraphNode *_node;
	command::CommandExecutionListener &_listener;
	ReferenceNodeCommandInterceptor(scenegraph::SceneGraphNode *node, command::CommandExecutionListener &listener)
		: _node(node), _listener(listener) {
	}
	bool allowed(const core::String &cmd, const core::DynamicArray<core::String> &args) override {
		if (_node->isReference()) {
			MainWindow::_popupModelUnreference = true;
			return false;
		}
		return _listener.allowed(cmd, args);
	}

	void operator()(const core::String &cmd, const core::DynamicArray<core::String> &args) override {
		_listener(cmd, args);
	}
};

bool ToolsPanel::init() {
	_gizmoOperations = core::Var::getSafe(cfg::VoxEditGizmoOperations);
	_showGizmoScene = core::Var::getSafe(cfg::VoxEditShowaxis);
	_showGizmoModel = core::Var::getSafe(cfg::VoxEditModelGizmo);
	return true;
}

void ToolsPanel::shutdown() {
}

void ToolsPanel::updateSceneMode(command::CommandExecutionListener &listener) {
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const int activeNode = sceneGraph.activeNode();

	if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(activeNode)) {
		const scenegraph::SceneGraphNodeType nodeType = node->type();
		if (ImGui::CollapsingHeader(_("Action"), ImGuiTreeNodeFlags_DefaultOpen)) {
			ui::ScopedStyle style;
			style.setFont(_app->bigIconFont());
			const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
			ui::Toolbar toolbar("scenetools", buttonSize, &listener);
			toolbar.button(ICON_LC_COPY, "nodeduplicate");
			toolbar.button(ICON_LC_TRASH, "nodedelete");
			if (nodeType == scenegraph::SceneGraphNodeType::Model) {
				toolbar.button(ICON_LC_COPY, "modelref");
				toolbar.button(ICON_LC_SHRINK, "center_origin");
				toolbar.button(ICON_LC_SHRINK, "center_referenceposition");
			} else if (nodeType == scenegraph::SceneGraphNodeType::ModelReference) {
				toolbar.button(ICON_LC_CODESANDBOX, "modelunref");
			}
			toolbar.button(ICON_LC_ALIGN_VERTICAL_DISTRIBUTE_CENTER, "align");
		}
	}
}

void ToolsPanel::updateEditMode(command::CommandExecutionListener &listener) {
	if (ImGui::CollapsingHeader(_("Action"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ui::ScopedStyle style;
		style.setFont(_app->bigIconFont());
		const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
		ui::Toolbar toolbar("edittools", buttonSize, &listener);
		toolbar.button(ICON_LC_CROP, "crop");
		toolbar.button(ICON_LC_SCALING, "resizetoselection", _sceneMgr->modifier().selections().empty());
		toolbar.button(ICON_LC_SPLIT, "splitobjects");
		toolbar.button(ICON_LC_EXPAND, "modelsize");
		toolbar.button(ICON_LC_UNGROUP, "colortomodel");
		toolbar.button(ICON_LC_SQUARE_CHEVRON_DOWN, "scaledown");
		toolbar.button(ICON_LC_SQUARE_CHEVRON_UP, "scaleup");
		toolbar.button(ICON_LC_PAINT_BUCKET, "fillhollow");
		toolbar.button(ICON_LC_ERASER, "hollow");
		toolbar.button(ICON_LC_X, "clear");
		toolbar.button(ICON_LC_PAINT_BUCKET, "fill");
	}

	const float buttonWidth = (float)_app->fontSize() * 4;
	if (ImGui::CollapsingHeader("Rotate on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("##rotatevolumeonaxis");
		veui::AxisButton(math::Axis::X, _("X"), "rotate x", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipTextUnformatted(_("Rotate by 90 degree on the x axis"));
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Y, _("Y"), "rotate y", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipTextUnformatted(_("Rotate by 90 degree on the y axis"));
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Z, _("Z"), "rotate z", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipTextUnformatted(_("Rotate by 90 degree on the z axis"));
		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader(_("Flip on axis"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("##flipvolumeonaxis");
		veui::AxisButton(math::Axis::X, ICON_LC_MOVE_HORIZONTAL " X", "flip x", nullptr, nullptr, buttonWidth,
						 &listener);
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Y, ICON_LC_MOVE_VERTICAL " Y", "flip y", nullptr, nullptr, buttonWidth,
						 &listener);
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Z, ICON_LC_MOVE_DIAGONAL " Z", "flip z", nullptr, nullptr, buttonWidth,
						 &listener);
		ImGui::PopID();
	}

	if (ImGui::IconCollapsingHeader(ICON_LC_ARROW_UP, _("Translate"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("##translatevolume");
		static glm::ivec3 translate{0};
		veui::InputAxisInt(math::Axis::X, _("X"), &translate.x, 1);
		veui::InputAxisInt(math::Axis::X, _("Y"), &translate.y, 1);
		veui::InputAxisInt(math::Axis::X, _("Z"), &translate.z, 1);
		const core::String &shiftCmd = core::string::format("shift %i %i %i", translate.x, translate.y, translate.z);
		ImGui::CommandIconButton(ICON_LC_GRID_3X3, _("Volumes"), shiftCmd.c_str(), listener);
		ImGui::SameLine();
		const core::String &moveCmd = core::string::format("move %i %i %i", translate.x, translate.y, translate.z);
		ImGui::CommandIconButton(ICON_LC_BOXES, _("Voxels"), moveCmd.c_str(), listener);
		ImGui::PopID();
	}

	if (ImGui::IconCollapsingHeader(ICON_LC_BOX, _("Cursor"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("##cursor");
		glm::ivec3 cursorPosition = _sceneMgr->modifier().cursorPosition();
		math::Axis lockedAxis = _sceneMgr->modifier().lockedAxis();
		if (veui::CheckboxAxisFlags(math::Axis::X, _("X"), &lockedAxis)) {
			command::executeCommands("lockx", &listener);
		}
		ImGui::TooltipCommand("lockx");
		ImGui::SameLine();
		const int step = core::Var::getSafe(cfg::VoxEditGridsize)->intVal();
		if (veui::InputAxisInt(math::Axis::X, "##cursorx", &cursorPosition.x, step)) {
			const core::String commandLine = core::string::format("cursor %i %i %i", cursorPosition.x, cursorPosition.y, cursorPosition.z);
			command::executeCommands(commandLine, &listener);
		}

		if (veui::CheckboxAxisFlags(math::Axis::Y, _("Y"), &lockedAxis)) {
			command::executeCommands("locky", &listener);
		}
		ImGui::TooltipCommand("locky");
		ImGui::SameLine();
		if (veui::InputAxisInt(math::Axis::Y, "##cursory", &cursorPosition.y, step)) {
			const core::String commandLine = core::string::format("cursor %i %i %i", cursorPosition.x, cursorPosition.y, cursorPosition.z);
			command::executeCommands(commandLine, &listener);
		}

		if (veui::CheckboxAxisFlags(math::Axis::Z, _("Z"), &lockedAxis)) {
			command::executeCommands("lockz", &listener);
		}
		ImGui::TooltipCommand("lockz");
		ImGui::SameLine();
		if (veui::InputAxisInt(math::Axis::Z, "##cursorz", &cursorPosition.z, step)) {
			const core::String commandLine = core::string::format("cursor %i %i %i", cursorPosition.x, cursorPosition.y, cursorPosition.z);
			command::executeCommands(commandLine, &listener);
		}
		ImGui::SliderVarInt(_("Cursor details"), cfg::VoxEditCursorDetails, 0, 2);
		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader(_("Text"))) {
		ImGui::InputText(_("Text"), &_text.input);

		ImGui::SetNextItemWidth(100.0f);
		if (ImGui::InputInt(ICON_LC_MOVE_VERTICAL "##textinput", &_text.size)) {
			_text.size = glm::clamp(_text.size, 6, 255);
		}
		ImGui::TooltipTextUnformatted(_("Font size"));
		ImGui::SameLine();

		ImGui::SetNextItemWidth(100.0f);
		ImGui::InputInt(ICON_LC_MOVE_HORIZONTAL "##textinput", &_text.spacing);
		ImGui::TooltipTextUnformatted(_("Horizontal spacing"));

		ImGui::SetNextItemWidth(100.0f);
		if (ImGui::InputInt(ICON_LC_EXPAND "##textinput", &_text.thickness)) {
			_text.thickness = glm::clamp(_text.thickness, 1, 255);
		}
		ImGui::TooltipTextUnformatted(_("Thickness"));

		ImGui::SameLine();
		ImGui::SetNextItemWidth(100.0f);
		ImGui::InputFile(_("Font"), &_text.font, io::format::fonts(), ImGuiInputTextFlags_ReadOnly);

		if (ImGui::Button(_("Execute"))) {
			_sceneMgr->renderText(_text.input.c_str(), _text.size, _text.thickness, _text.spacing, _text.font.c_str());
		}
	}
}

void ToolsPanel::update(const char *title, bool sceneMode, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		if (sceneMode) {
			updateSceneMode(listener);
		} else {
			const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
			const int activeNode = sceneGraph.activeNode();
			if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(activeNode)) {
				ReferenceNodeCommandInterceptor wrapper(node, listener);
				updateEditMode(wrapper);
			}
		}

		if (ImGui::IconCollapsingHeader(ICON_LC_BOX, _("Gizmo settings"), ImGuiTreeNodeFlags_DefaultOpen)) {
			const core::VarPtr &gizmoVar = sceneMode ? _showGizmoScene : _showGizmoModel;
			ImGui::IconCheckboxVar(ICON_LC_AXIS_3D, _("Show gizmo"), gizmoVar);

			ImGui::Indent();
			if (!gizmoVar->boolVal())
				ImGui::BeginDisabled();

			if (sceneMode) {
				int operations = _gizmoOperations->intVal();
				bool dirty = false;

				dirty |= ImGui::IconCheckboxFlags(ICON_LC_ROTATE_3D, _("Rotate"), &operations, GizmoOperation_Rotate);
				ImGui::TooltipTextUnformatted(_("Activate the rotate operation"));

				dirty |= ImGui::IconCheckboxFlags(ICON_LC_MOVE_3D, _("Translate"), &operations, GizmoOperation_Translate);
				ImGui::TooltipTextUnformatted(_("Activate the translate operation"));

				// dirty |= ImGui::IconCheckboxFlags(ICON_LC_BOX, _("Bounds"), &operations, GizmoOperation_Bounds);
				// ImGui::TooltipTextUnformatted(_("Activate the bounds operation"));

				// dirty |= ImGui::IconCheckboxFlags(ICON_LC_SCALE_3D, _("Scale"), &operations, GizmoOperation_Scale);
				// ImGui::TooltipTextUnformatted(_("Activate the uniform scale operation"));

				if (dirty) {
					_gizmoOperations->setVal(operations);
				}
				ImGui::IconCheckboxVar(ICON_LC_REFRESH_CCW_DOT, _("Pivot"), cfg::VoxEditGizmoPivot);
			}
			ImGui::IconCheckboxVar(ICON_LC_MAGNET, _("Snap to grid"), cfg::VoxEditGizmoSnap);
			ImGui::IconCheckboxVar(ICON_LC_FLIP_HORIZONTAL_2, _("Flip axis"), cfg::VoxEditGizmoAllowAxisFlip);

			if (!gizmoVar->boolVal())
				ImGui::EndDisabled();

			ImGui::Unindent();
		}
	}
	ImGui::End();
}

} // namespace voxedit
