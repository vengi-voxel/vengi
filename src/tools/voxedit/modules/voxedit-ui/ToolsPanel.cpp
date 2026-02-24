/**
 * @file
 */

#include "ToolsPanel.h"
#include "command/CommandHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedID.h"
#include "ui/Toolbar.h"
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
		if (_node->isReferenceNode()) {
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
	_gizmoOperations = core::getVar(cfg::VoxEditGizmoOperations);
	_showGizmoScene = core::getVar(cfg::VoxEditShowaxis);
	_showGizmoModel = core::getVar(cfg::VoxEditModelGizmo);
	_localSpace = core::getVar(cfg::VoxEditLocalSpace);
	_cursorDetails = core::getVar(cfg::VoxEditCursorDetails);
	_gridSize = core::getVar(cfg::VoxEditGridsize);
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
			style.pushFontSize(imguiApp()->bigFontSize());
			ui::Toolbar toolbar("toolbar", &listener);
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
		style.pushFontSize(imguiApp()->bigFontSize());
		ui::Toolbar toolbar("toolbar", &listener);
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->sceneGraph().activeNode());
		const bool hasSelection = node && node->hasSelection();
		toolbar.button(ICON_LC_CROP, "crop");
		toolbar.button(ICON_LC_SCALING, "resizetoselection", !hasSelection);
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

	const float buttonWidth = ImGui::GetFontSize() * 4;
	if (ImGui::CollapsingHeader(_("Rotate on axis"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ui::ScopedID id("##rotatevolumeonaxis");
		ImGui::AxisCommandButton(math::Axis::X, _("X"), "rotate x", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipTextUnformatted(_("Rotate by 90 degree on the x axis"));
		ImGui::SameLine();
		ImGui::AxisCommandButton(math::Axis::Y, _("Y"), "rotate y", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipTextUnformatted(_("Rotate by 90 degree on the y axis"));
		ImGui::SameLine();
		ImGui::AxisCommandButton(math::Axis::Z, _("Z"), "rotate z", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipTextUnformatted(_("Rotate by 90 degree on the z axis"));
	}

	if (ImGui::CollapsingHeader(_("Flip on axis"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ui::ScopedID id("##flipvolumeonaxis");
		ImGui::AxisCommandButton(math::Axis::X, _("X"), "flip x", ICON_LC_MOVE_HORIZONTAL, nullptr, buttonWidth,
						 &listener);
		ImGui::SameLine();
		ImGui::AxisCommandButton(math::Axis::Y, _("Y"), "flip y", ICON_LC_MOVE_VERTICAL, nullptr, buttonWidth,
						 &listener);
		ImGui::SameLine();
		ImGui::AxisCommandButton(math::Axis::Z, _("Z"), "flip z", ICON_LC_MOVE_DIAGONAL, nullptr, buttonWidth,
						 &listener);
	}

	if (ImGui::IconCollapsingHeader(ICON_LC_ARROW_UP, _("Move voxels"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ui::ScopedID id("##movevoxels");
		static glm::ivec3 translate{0};
		const int minStep = _gridSize->intVal();
		ImGui::InputAxisInt(math::Axis::X, _("X"), &translate.x, minStep);
		ImGui::InputAxisInt(math::Axis::X, _("Y"), &translate.y, minStep);
		ImGui::InputAxisInt(math::Axis::X, _("Z"), &translate.z, minStep);

		const core::String &moveCmd = core::String::format("move %i %i %i", translate.x, translate.y, translate.z);
		ImGui::CommandIconButton(ICON_LC_BOXES, _("Move"), moveCmd.c_str(), listener);
	}

	if (ImGui::IconCollapsingHeader(ICON_LC_BOX, _("Cursor"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ui::ScopedID id("##cursor");
		glm::ivec3 cursorPosition = _sceneMgr->modifier().cursorPosition();
		math::Axis lockedAxis = _sceneMgr->modifier().lockedAxis();
		if (ImGui::CheckboxAxisFlags(math::Axis::X, _("X"), &lockedAxis)) {
			command::executeCommands("lockx", &listener);
		}
		ImGui::TooltipCommand("lockx");
		ImGui::SameLine();
		const int step = core::getVar(cfg::VoxEditGridsize)->intVal();
		if (ImGui::InputAxisInt(math::Axis::X, "##cursorx", &cursorPosition.x, step)) {
			const core::String commandLine = core::String::format("cursor %i %i %i", cursorPosition.x, cursorPosition.y, cursorPosition.z);
			command::executeCommands(commandLine, &listener);
		}

		if (ImGui::CheckboxAxisFlags(math::Axis::Y, _("Y"), &lockedAxis)) {
			command::executeCommands("locky", &listener);
		}
		ImGui::TooltipCommand("locky");
		ImGui::SameLine();
		if (ImGui::InputAxisInt(math::Axis::Y, "##cursory", &cursorPosition.y, step)) {
			const core::String commandLine = core::String::format("cursor %i %i %i", cursorPosition.x, cursorPosition.y, cursorPosition.z);
			command::executeCommands(commandLine, &listener);
		}

		if (ImGui::CheckboxAxisFlags(math::Axis::Z, _("Z"), &lockedAxis)) {
			command::executeCommands("lockz", &listener);
		}
		ImGui::TooltipCommand("lockz");
		ImGui::SameLine();
		if (ImGui::InputAxisInt(math::Axis::Z, "##cursorz", &cursorPosition.z, step)) {
			const core::String commandLine = core::String::format("cursor %i %i %i", cursorPosition.x, cursorPosition.y, cursorPosition.z);
			command::executeCommands(commandLine, &listener);
		}

		const char *cursorDetails[] = {_("Disabled"), _("Position"), _("Details"), _("Distance")};
		const int cursorDetailValue = _cursorDetails->intVal();
		const char *cursorDetailName = (cursorDetailValue >= 0 && cursorDetailValue < lengthof(cursorDetails))
										   ? cursorDetails[cursorDetailValue]
										   : _("Unknown");
		ImGui::SliderVarInt(_cursorDetails, cursorDetailName);
	}
}

void ToolsPanel::update(const char *id, bool sceneMode, command::CommandExecutionListener &listener) {
	core_trace_scoped(ToolsPanel);
	const core::String title = makeTitle(ICON_LC_WRENCH, _("Tools"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
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
			ImGui::IconCheckboxVar(ICON_LC_AXIS_3D, gizmoVar);

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
				ImGui::IconCheckboxVar(ICON_LC_REFRESH_CCW_DOT, cfg::VoxEditGizmoPivot);
			}
			ImGui::IconCheckboxVar(ICON_LC_MAGNET, cfg::VoxEditGizmoSnap);
			ImGui::IconCheckboxVar(ICON_LC_FLIP_HORIZONTAL_2, cfg::VoxEditGizmoAllowAxisFlip);
			ImGui::CheckboxVar(_localSpace);

			if (!gizmoVar->boolVal())
				ImGui::EndDisabled();

			ImGui::Unindent();
		}
	}
	ImGui::End();
}

} // namespace voxedit
