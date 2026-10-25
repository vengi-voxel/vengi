/**
 * @file
 */

#include "NormalPalettePanel.h"
#include "app/I18N.h"
#include "color/Color.h"
#include "command/CommandHandler.h"
#include "core/Trace.h"
#include "math/Math.h"
#include "memento/MementoHandler.h"
#include "palette/NormalPalette.h"
#include "palette/PaletteFormatDescription.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedPanel.h"
#include "ui/dearimgui/imgui_internal.h"
#include "video/Camera.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

NormalPalettePanel::NormalPalettePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr)
	: Super(app, "normalpalette"), _sceneMgr(sceneMgr), _redColor(ImGui::GetColorU32(color::Red())),
	  _yellowColor(ImGui::GetColorU32(color::Yellow())), _darkRedColor(ImGui::GetColorU32(color::DarkRed())) {
}

void NormalPalettePanel::init() {
	_renderNormals = core::getVar(cfg::RenderNormals);
}

void NormalPalettePanel::addColor(ImVec2 &cursorPos, float startingPosX, float contentRegionRightEdge,
								  uint8_t paletteColorIdx, float colorButtonSize, scenegraph::SceneGraphNode &node,
								  command::CommandExecutionListener &listener) {
	core_trace_scoped(AddColor);
	palette::NormalPalette &normalPalette = node.normalPalette();
	const int maxPaletteEntries = normalPalette.size();
	const float borderWidth = 1.0f;
	ImDrawList *drawList = ImGui::GetWindowDrawList();

	const ImVec2 v1(cursorPos.x + borderWidth, cursorPos.y + borderWidth);
	const ImVec2 v2(cursorPos.x + colorButtonSize, cursorPos.y + colorButtonSize);
	const bool existingColor = paletteColorIdx < maxPaletteEntries;
	color::RGBA color;
	if (existingColor) {
		color = normalPalette.normal(paletteColorIdx);
		if (color.a != 255) {
			color::RGBA own = color;
			own.a = 127;
			color::RGBA other = color;
			other.a = 255;
			drawList->AddRectFilledMultiColor(v1, v2, own, own, own, other);
		} else {
			drawList->AddRectFilled(v1, v2, color);
		}
	} else {
		color = color::RGBA(0, 0, 0, 0);
		drawList->AddRect(v1, v2, color::RGBA(0, 0, 0, 255));
	}

	const bool usableColor = existingColor && color.a > 0;
	const core::String label = core::String::format("normal-%i", paletteColorIdx);
	const ImGuiID id = ImGui::GetID(label.c_str());
	const ImRect bb(cursorPos, ImVec2(cursorPos.x + colorButtonSize, cursorPos.y + colorButtonSize));
	bool hovered = false;
	bool held = false;
	if (ImGui::ItemAdd(bb, id)) {
		if (ImGui::ButtonBehavior(bb, id, &hovered, &held)) {
			selectNormal(normalPalette, paletteColorIdx);
		}
		struct DragNormal {
			core::UUID nodeUUID;
			uint8_t index;
		};
		if (usableColor && ImGui::BeginDragDropSource()) {
			const DragNormal payload{node.uuid(), paletteColorIdx};
			ImGui::SetDragDropPayload("normalpalette-entry", &payload, sizeof(payload));
			ImGui::TextUnformatted(_("Swap entries and preserve painted directions"));
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("normalpalette-entry")) {
				const DragNormal &source = *(const DragNormal *)payload->Data;
				if (source.nodeUUID == node.uuid() &&
					_sceneMgr->nodeSwapNormals(node.uuid(), source.index, paletteColorIdx)) {
					selectNormal(normalPalette, paletteColorIdx);
				}
			}
			ImGui::EndDragDropTarget();
		}
		if (hovered) {
			if (usableColor) {
				const glm::vec3 direction = glm::normalize(normalPalette.normal3f(paletteColorIdx));
				ImGui::SetTooltip(_("Normal %i: %.3f, %.3f, %.3f"), paletteColorIdx, direction.x, direction.y,
								  direction.z);
			} else {
				ImGui::SetItemTooltipUnformatted(_("Empty normal slot - select to create a direction"));
			}
			if (usableColor && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C)) {
				_copiedNormal = normalPalette.normal(paletteColorIdx);
				_hasCopiedNormal = true;
			} else if (_hasCopiedNormal && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_V)) {
				if (_sceneMgr->nodeSetNormal(node.uuid(), paletteColorIdx, _copiedNormal)) {
					selectNormal(normalPalette, paletteColorIdx);
				}
			}
		}
		handleContextMenu(paletteColorIdx, node);
	}
	if (hovered) {
		drawList->AddRect(v1, v2, _redColor, 0.0f, 2.0f);
	} else if (_selectedIndex == paletteColorIdx) {
		drawList->AddRect(v1, v2, _darkRedColor, 0.0f, 2.0f);
	} else if (paletteColorIdx == currentSceneNormal() && usableColor) {
		drawList->AddRect(v1, v2, _yellowColor, 0.0f, 2.0f);
	}

	cursorPos.x += colorButtonSize;
	if (cursorPos.x > contentRegionRightEdge - colorButtonSize) {
		cursorPos.x = startingPosX;
		cursorPos.y += colorButtonSize;
	}
}

void NormalPalettePanel::selectNormal(const palette::NormalPalette &palette, uint8_t index) {
	finishNormalEdit();
	_selectedIndex = index;
	_targetValid = true;
	if (index < palette.size() && palette.normal(index).a > 0) {
		_targetNormal = glm::normalize(palette.normal3f(index));
		_sceneMgr->modifier().setNormalColorIndex(index);
	} else {
		_targetNormal = glm::vec3(0.0f, 1.0f, 0.0f);
	}
}

void NormalPalettePanel::finishNormalEdit() {
	if (!_normalChangePending) {
		return;
	}
	if (const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(_editingNodeUUID)) {
		_sceneMgr->mementoHandler().markNormalPaletteChange(_sceneMgr->sceneGraph(), *node);
	}
	_normalChangePending = false;
}

bool NormalPalettePanel::applyNormal(scenegraph::SceneGraphNode &node) {
	if (!_targetValid || _selectedIndex < 0 || _selectedIndex >= palette::NormalPaletteMaxNormals) {
		return false;
	}
	if (!_sceneMgr->nodeSetNormal(node.uuid(), (uint8_t)_selectedIndex,
								  palette::NormalPalette::toRGBA(_targetNormal))) {
		return false;
	}
	selectNormal(node.normalPalette(), (uint8_t)_selectedIndex);
	return true;
}

void NormalPalettePanel::handleContextMenu(uint8_t index, scenegraph::SceneGraphNode &node) {
	const core::String popup = core::String::format("normal-context-%i", index);
	if (!ImGui::BeginPopupContextItem(popup.c_str())) {
		return;
	}
	const palette::NormalPalette &palette = node.normalPalette();
	const bool populated = index < palette.size() && palette.normal(index).a > 0;
	if (ImGui::IconMenuItem(ICON_LC_COPY, _("Copy normal"), nullptr, false, populated)) {
		_copiedNormal = palette.normal(index);
		_hasCopiedNormal = true;
	}
	if (ImGui::IconMenuItem(ICON_LC_CLIPBOARD_PASTE, _("Paste normal"), nullptr, false, _hasCopiedNormal)) {
		if (_sceneMgr->nodeSetNormal(node.uuid(), index, _copiedNormal)) {
			selectNormal(palette, index);
		}
	}
	int freeIndex = -1;
	for (int i = 0; i < palette::NormalPaletteMaxNormals; ++i) {
		if (palette.normal(i).a == 0) {
			freeIndex = i;
			break;
		}
	}
	if (ImGui::IconMenuItem(ICON_LC_COPY_PLUS, _("Duplicate normal"), nullptr, false, populated && freeIndex >= 0)) {
		if (_sceneMgr->nodeSetNormal(node.uuid(), (uint8_t)freeIndex, palette.normal(index))) {
			selectNormal(palette, (uint8_t)freeIndex);
		}
	}
	ImGui::EndPopup();
}

void NormalPalettePanel::paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	if (!ImGui::IsAnyItemActive()) {
		finishNormalEdit();
	}
	palette::NormalPalette &normalPalette = node.normalPalette();
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginIconMenu(ICON_LC_PALETTE, _("File"))) {
			if (ImGui::MenuItem(_("Tiberian Sun"))) {
				const core::String &cmd = core::String::format("loadnormalpalette %s", palette::NormalPalette::builtIn[1]);
				command::executeCommands(cmd, &listener);
			}
			if (ImGui::MenuItem(_("Red Alert 2"))) {
				const core::String &cmd = core::String::format("loadnormalpalette %s", palette::NormalPalette::builtIn[0]);
				command::executeCommands(cmd, &listener);
			}
			if (ImGui::MenuItem(_("Slab6"))) {
				const core::String &cmd = core::String::format("loadnormalpalette %s", palette::NormalPalette::builtIn[2]);
				command::executeCommands(cmd, &listener);
			}
			if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Export"))) {
				_app->saveDialog([&](const core::String &file,
									 const io::FormatDescription *desc) { normalPalette.save(file.c_str()); },
								 {}, palette::palettes(), "palette.png");
			}
			if (ImGui::BeginMenu(_("Auto normals"))) {
				const char *normalModes[] = {_("Flat"), _("Smooth"), _("Smoother")};
				const core::VarPtr &normalModeVar = core::getVar(cfg::VoxEditAutoNormalMode);
				const int currentNormalMode = normalModeVar->intVal();

				if (ImGui::BeginCombo(_("Normal mode"), normalModes[currentNormalMode])) {
					for (int i = 0; i < lengthof(normalModes); ++i) {
						const char *normalMode = normalModes[i];
						if (normalMode == nullptr) {
							continue;
						}
						const bool selected = i == currentNormalMode;
						if (ImGui::Selectable(normalMode, selected)) {
							normalModeVar->setVal(core::string::toString(i));
						}
						if (selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::Checkbox(_("Recalculate all normals"), &_recalcAll);
				ImGui::SetItemTooltipUnformatted(_("If the model already has normals and you want to replace them"));
				ImGui::Checkbox(_("Model is hollow"), &_onlySurfaceVoxels);
				ImGui::SetItemTooltipUnformatted(
					_("Fill hollows to re-calculate the normals and\nhollow the model afterwards again.\n\n"
					  "For calculating normals it is needed that the model has a closed\n"
					  "surface and the hollow area is filled.\n\n"
					  "Either do it manually or activate this option."));
				if (ImGui::IconMenuItem(ICON_LC_PLAY, _("Calculate normals"))) {
					voxel::Connectivity connectivity = voxel::Connectivity::SixConnected;
					if (currentNormalMode == 1) {
						connectivity = voxel::Connectivity::EighteenConnected;
					} else if (currentNormalMode == 2) {
						connectivity = voxel::Connectivity::TwentySixConnected;
					}
					_sceneMgr->nodeGroupCalulateNormals(connectivity, _recalcAll, _onlySurfaceVoxels);
				}

				ImGui::EndMenu();
			}
			ImGui::TooltipTextUnformatted(_("Calculate normals for the model"));
			ImGui::CommandMenuItem(_("Remove all normals"), "normpalette_removenormals", node.id(), &listener);
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Options"))) {
			ImGui::SliderVarFloat(cfg::VoxEditPaletteColorSize);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

uint8_t NormalPalettePanel::currentSceneNormal() const {
	const uint8_t normal = _sceneMgr->hitCursorVoxel().getNormal();
	return normal == NO_NORMAL ? palette::NO_NORMAL_REMAP_FOUND : normal - NORMAL_PALETTE_OFFSET;
}

void NormalPalettePanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(NormalPalettePanel);
	static ui::ScopedPanel panel(cfg::VoxEditShowNormalPalette);
	if (!panel.isOpen()) {
		finishNormalEdit();
		return;
	}
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const int nodeId = sceneGraph.activeNode();
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float contentRegionHeight = available.y + ImGui::GetCursorPosY();
	const ImVec2 windowSize(10.0f * ImGui::GetFrameHeight(), contentRegionHeight);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
	const core::String &title = makeTitle(ICON_LC_PALETTE, _("Normals"), id);
	if (ui::ScopedPanel::Scope scope = panel.begin(title.c_str(), ImGuiWindowFlags_MenuBar)) {
		if (node.isModelNode()) {
			const palette::NormalPalette &normals = node.normalPalette();
			if (_lastNodeUUID != node.uuid() || _lastPaletteHash != normals.hash()) {
				if (_lastNodeUUID != node.uuid()) {
					_selectedIndex = _sceneMgr->modifier().normalColorIndex();
				}
				_selectedIndex = core_min(core_max(_selectedIndex, 0), palette::NormalPaletteMaxNormals - 1);
				const uint8_t brush = _sceneMgr->modifier().normalColorIndex();
				if (brush >= normals.size() || normals.normal(brush).a == 0) {
					const int match = normals.getClosestMatch(glm::vec3(0, 1, 0));
					if (match != palette::PaletteNormalNotFound) {
						_sceneMgr->modifier().setNormalColorIndex((uint8_t)match);
					}
				}
				selectNormal(normals, (uint8_t)_selectedIndex);
				_lastNodeUUID = node.uuid();
				_lastPaletteHash = normals.hash();
			}
			paletteMenuBar(node, listener);
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();
			const float startingPosX = cursorPos.x;
			const float availableX = ImGui::GetContentRegionAvail().x;
			const float contentRegionRightEdge = availableX + cursorPos.x;
			ImDrawList *drawList = ImGui::GetWindowDrawList();
			const ImDrawFlags backupFlags = drawList->Flags;
			drawList->Flags &= ~ImDrawFlags_AALines;
			const float frameHeight = ImGui::GetFrameHeight();
			const float cellSize = frameHeight * core::getVar(cfg::VoxEditPaletteColorSize)->floatVal();

			const int columns = core_max(1, (int)(availableX / cellSize));
			const ImVec2 origin = cursorPos;
			for (int index = 0; index < palette::NormalPaletteMaxNormals; ++index) {
				cursorPos = ImVec2(origin.x + (index % columns) * cellSize, origin.y + (index / columns) * cellSize);
				addColor(cursorPos, startingPosX, contentRegionRightEdge, (uint8_t)index, cellSize, node, listener);
			}
			const int rows = (palette::NormalPaletteMaxNormals + columns - 1) / columns;
			ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + rows * cellSize));

			drawList->Flags = backupFlags;
			ImGui::Dummy(ImVec2(0, frameHeight));

			const int sceneHoveredPaletteNormalIdx = currentSceneNormal();
			ImGui::Text(_("Normal index: %i (scene normal index %i)"), _selectedIndex,
						sceneHoveredPaletteNormalIdx == palette::NO_NORMAL_REMAP_FOUND ? -1
																					   : sceneHoveredPaletteNormalIdx);

			ImGui::CheckboxVar(_renderNormals);

			drawNormalVisualization(normals);

			if (ImGui::InputFloat3(_("Normal"), glm::value_ptr(_targetNormal))) {
				setTargetNormal(_targetNormal);
			}
			float longitude, latitude;
			math::vectorToPolar(_targetValid ? _targetNormal : glm::vec3(0.0f, 1.0f, 0.0f), longitude, latitude);
			bool polarChanged = false;
			polarChanged |= ImGui::SliderAngle(_("Longitude"), &longitude, -180, 179, "%.0f");
			polarChanged |= ImGui::SliderAngle(_("Latitude"), &latitude, -90, 89, "%.0f");
			if (polarChanged) {
				setTargetNormal(math::polarToVector(longitude, latitude));
			}

			ImGui::TextUnformatted(_("Flip"));
			ImGui::PushID("##flipnormal");
			if (ImGui::AxisButtonX()) {
				setTargetNormal(glm::vec3(-_targetNormal.x, _targetNormal.y, _targetNormal.z));
			}
			ImGui::SameLine();
			if (ImGui::AxisButtonY()) {
				setTargetNormal(glm::vec3(_targetNormal.x, -_targetNormal.y, _targetNormal.z));
			}
			ImGui::SameLine();
			if (ImGui::AxisButtonZ()) {
				setTargetNormal(glm::vec3(_targetNormal.x, _targetNormal.y, -_targetNormal.z));
			}
			ImGui::PopID();

			ImGui::TextUnformatted(_("Rotate 90 deg"));
			ImGui::PushID("##rotnormal");
			if (ImGui::AxisButtonX()) {
				setTargetNormal(glm::vec3(_targetNormal.x, _targetNormal.z, -_targetNormal.y));
			}
			ImGui::SameLine();
			if (ImGui::AxisButtonY()) {
				setTargetNormal(glm::vec3(_targetNormal.z, _targetNormal.y, -_targetNormal.x));
			}
			ImGui::SameLine();
			if (ImGui::AxisButtonZ()) {
				setTargetNormal(glm::vec3(_targetNormal.y, -_targetNormal.x, _targetNormal.z));
			}
			ImGui::PopID();
			ImGui::TextUnformatted(_("Direction"));
			const char *axisNames[] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
			const glm::vec3 axes[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
			for (int i = 0; i < 6; ++i) {
				if (i > 0) {
					ImGui::SameLine();
				}
				if (ImGui::Button(axisNames[i])) {
					setTargetNormal(axes[i]);
				}
			}
			if (!_targetValid) {
				ImGui::TextUnformatted(_("Enter a finite, non-zero normal."));
			}
			const bool populated = _selectedIndex < (int)normals.size() && normals.normal(_selectedIndex).a > 0;
			if (!populated && ImGui::DisabledButton(_("Add normal"), !_targetValid || _sceneMgr->isLocked())) {
				applyNormal(node);
			}
			ImGui::TextWrapped(_("Direction edits affect every voxel using this entry."));
		}
	}
	if (!ImGui::IsAnyItemActive()) {
		finishNormalEdit();
	}
}

void NormalPalettePanel::drawNormalVisualization(const palette::NormalPalette &normalPalette) {
	video::Camera *camera = _sceneMgr->activeCamera();
	if (camera == nullptr) {
		return;
	}

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	const float dpiScale = ImGui::GetStyle().FontScaleDpi;
	const float radius = 40.0f * dpiScale;
	const float padding = 10.0f * dpiScale;

	ImVec2 cursorPos = ImGui::GetCursorScreenPos();
	const ImVec2 centerFace(cursorPos.x + radius, cursorPos.y + radius);
	const ImVec2 centerBack(cursorPos.x + radius * 3 + padding, cursorPos.y + radius);

	const int steps = 10;
	for (int i = 0; i < steps; ++i) {
		float t = (float)i / (float)(steps - 1);
		float r = radius * (1.0f - t);
		int valFace = 100 + (int)(155.0f * t);
		drawList->AddCircleFilled(centerFace, r, IM_COL32(valFace, valFace, valFace, 255));
		int valBack = 50 + (int)(100.0f * t);
		drawList->AddCircleFilled(centerBack, r, IM_COL32(valBack, valBack, valBack, 255));
	}
	drawList->AddCircle(centerFace, radius, IM_COL32(255, 255, 255, 255));
	drawList->AddText(ImVec2(centerFace.x - 15, centerFace.y + radius + 2), IM_COL32(255, 255, 255, 255), _("Front"));

	drawList->AddCircle(centerBack, radius, IM_COL32(255, 255, 255, 255));
	drawList->AddText(ImVec2(centerBack.x - 15, centerBack.y + radius + 2), IM_COL32(255, 255, 255, 255), _("Back"));

	ImGui::Dummy(ImVec2(radius * 4 + padding, radius * 2 + 20));

	if (!_targetValid) {
		return;
	}
	const glm::mat4 &viewMatrix = camera->viewMatrix();
	const glm::vec3 viewNormal = glm::mat3(viewMatrix) * _targetNormal;

	const bool facing = viewNormal.z > 0.0f;
	const ImVec2 center = facing ? centerFace : centerBack;
	const ImVec2 pinPos(center.x + viewNormal.x * radius, center.y - viewNormal.y * radius);

	drawList->AddLine(center, pinPos, IM_COL32(255, 0, 0, 255));
	drawList->AddCircleFilled(pinPos, 4.0f * dpiScale, IM_COL32(255, 0, 0, 255));

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		const ImVec2 mousePos = ImGui::GetMousePos();
		bool clicked = false;
		bool front = true;
		float x = 0.0f;
		float y = 0.0f;

		auto checkCircle = [&](const ImVec2 &c, bool f) {
			const float dx = mousePos.x - c.x;
			const float dy = c.y - mousePos.y; // Invert Y
			if (dx * dx + dy * dy <= radius * radius) {
				clicked = true;
				front = f;
				x = dx / radius;
				y = dy / radius;
			}
		};

		checkCircle(centerFace, true);
		checkCircle(centerBack, false);

		if (clicked) {
			float z = glm::sqrt(core_max(0.0f, 1.0f - x * x - y * y));
			if (!front) {
				z = -z;
			}
			// Transform back to world space
			// We need inverse view matrix (rotation only)
			// Inverse of rotation matrix is transpose
			const glm::mat3 invViewMatrix = glm::transpose(glm::mat3(viewMatrix));
			const glm::vec3 newNormal = invViewMatrix * glm::vec3(x, y, z);
			setTargetNormal(glm::normalize(newNormal));
		}
	}
}

void NormalPalettePanel::setTargetNormal(const glm::vec3 &normal) {
	_targetNormal = normal;
	const float lengthSquared = glm::dot(normal, normal);
	_targetValid = !glm::any(glm::isnan(normal)) && !glm::any(glm::isinf(normal)) && !glm::isnan(lengthSquared) &&
				   !glm::isinf(lengthSquared) && lengthSquared > 0.000001f;
	if (!_targetValid) {
		return;
	}
	_targetNormal = glm::normalize(normal);
	if (_selectedIndex < 0 || _selectedIndex >= palette::NormalPaletteMaxNormals || _sceneMgr->isLocked()) {
		return;
	}
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(_sceneMgr->activeNode());
	if (node == nullptr || !node->isModelNode()) {
		return;
	}
	const color::RGBA encoded = palette::NormalPalette::toRGBA(_targetNormal);
	palette::NormalPalette &palette = node->normalPalette();
	if (palette.normal((uint8_t)_selectedIndex) == encoded) {
		return;
	}
	if (_normalChangePending && _editingNodeUUID != node->uuid()) {
		finishNormalEdit();
	}
	palette.setNormal((uint8_t)_selectedIndex, encoded);
	palette.markSave();
	_sceneMgr->modifier().setNormalColorIndex((uint8_t)_selectedIndex);
	_sceneMgr->markDirty();
	_editingNodeUUID = node->uuid();
	_normalChangePending = true;
	_lastPaletteHash = palette.hash();
}

} // namespace voxedit
