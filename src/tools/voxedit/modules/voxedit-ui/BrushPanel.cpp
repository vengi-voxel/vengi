/**
 * @file
 */

#include "BrushPanel.h"
#include "DragAndDropPayload.h"
#include "ScopedStyle.h"
#include "Toolbar.h"
#include "core/Algorithm.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IconsForkAwesome.h"
#include "voxedit-ui/Util.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxedit-util/modifier/brush/StampBrush.h"
#include "voxedit-util/tool/Clipboard.h"
#include "voxel/Face.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

static constexpr const char *BrushTypeIcons[] = {ICON_FA_CUBES, ICON_FA_CODE, ICON_FA_LAYER_GROUP, ICON_FA_STAMP};
static_assert(lengthof(BrushTypeIcons) == (int)BrushType::Max, "BrushTypeIcons size mismatch");

void BrushPanel::reloadScriptParameters(const core::String &script) {
	_scriptParameterDescription.clear();
	ScriptBrush &scriptBrush = sceneMgr().modifier().scriptBrush();
	scriptBrush.luaGenerator().argumentInfo(script, _scriptParameterDescription);
	const int parameterCount = (int)_scriptParameterDescription.size();
	_scriptParameters.clear();
	_scriptParameters.resize(parameterCount);
	_enumValues.clear();
	_enumValues.resize(parameterCount);
	for (int i = 0; i < parameterCount; ++i) {
		const voxelgenerator::LUAParameterDescription &p = _scriptParameterDescription[i];
		_scriptParameters[i] = p.defaultValue;
		_enumValues[i] = p.enumValues;
	}
}

void BrushPanel::updateScriptBrushPanel(command::CommandExecutionListener &listener) {
	if (_scripts.empty()) {
		ScriptBrush &scriptBrush = sceneMgr().modifier().scriptBrush();
		_scripts = scriptBrush.luaGenerator().listScripts();
	}
	if (_scripts.empty()) {
		return;
	}

	if (ImGui::ComboStl("##script", &_currentScript, _scripts)) {
		if (_currentScript >= 0 && _currentScript < (int)_scripts.size()) {
			const core::String &scriptName = _scripts[_currentScript].filename;
			ScriptBrush &scriptBrush = sceneMgr().modifier().scriptBrush();
			_activeScript = scriptBrush.luaGenerator().load(scriptName);
			reloadScriptParameters(_activeScript);
		}
	}
	ImGui::TooltipText("LUA scripts for manipulating the voxel volumes");

	ImGui::SameLine();

	const bool validScriptIndex = _currentScript >= 0 && _currentScript < (int)_scripts.size();
	const bool validScript = validScriptIndex && _scripts[_currentScript].valid;
	if (ImGui::DisabledButton("Run##scriptpanel", !validScript)) {
		sceneMgr().runScript(_activeScript, _scriptParameters);
		core::DynamicArray<core::String> args;
		args.reserve(_scriptParameters.size() + 1);
		args.push_back(_scripts[_currentScript].filename);
		args.append(_scriptParameters);
		listener("xs", _scriptParameters);
	}
	ImGui::TooltipText("Execute the selected script for the currently loaded voxel volumes");

	const int n = (int)_scriptParameterDescription.size();
	if (n && ImGui::CollapsingHeader("Script parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
		for (int i = 0; i < n; ++i) {
			const voxelgenerator::LUAParameterDescription &p = _scriptParameterDescription[i];
			switch (p.type) {
			case voxelgenerator::LUAParameterType::ColorIndex: {
				const voxel::Palette &palette = sceneMgr().activePalette();
				core::String &str = _scriptParameters[i];
				int val = core::string::toInt(str);
				if (val >= 0 && val < palette.colorCount()) {
					const float size = 20;
					const ImVec2 v1 = ImGui::GetCursorScreenPos();
					const ImVec2 v2(v1.x + size, v1.y + size);
					ImDrawList *drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(palette.color(val)));
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size);
				}
				if (ImGui::InputInt(p.name.c_str(), &val)) {
					if (val >= 0 && val < palette.colorCount()) {
						str = core::string::toString(val);
					}
				}

				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::PaletteIndexPayload)) {
						const int palIdx = *(const uint8_t *)payload->Data;
						str = core::string::toString(palIdx);
					}
					ImGui::EndDragDropTarget();
				}

				break;
			}
			case voxelgenerator::LUAParameterType::Integer: {
				core::String &str = _scriptParameters[i];
				int val = core::string::toInt(str);
				if (p.shouldClamp()) {
					int maxVal = (int)(p.maxValue + glm::epsilon<double>());
					int minVal = (int)(p.minValue + glm::epsilon<double>());
					if (ImGui::SliderInt(p.name.c_str(), &val, minVal, maxVal)) {
						str = core::string::toString(val);
					}
				} else if (ImGui::InputInt(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Float: {
				core::String &str = _scriptParameters[i];
				float val = core::string::toFloat(str);
				if (p.shouldClamp()) {
					const float maxVal = (float)p.maxValue;
					const float minVal = (float)p.minValue;
					const char *format;
					if (glm::abs(maxVal - minVal) <= 10.0f) {
						format = "%.6f";
					} else {
						format = "%.3f";
					}

					if (ImGui::SliderFloat(p.name.c_str(), &val, minVal, maxVal, format)) {
						str = core::string::toString(val);
					}
				} else if (ImGui::InputFloat(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::String: {
				core::String &str = _scriptParameters[i];
				ImGui::InputText(p.name.c_str(), &str);
				break;
			}
			case voxelgenerator::LUAParameterType::Enum: {
				core::String &str = _scriptParameters[i];
				core::DynamicArray<core::String> tokens;
				core::string::splitString(_enumValues[i], tokens, ",");
				const auto i = core::find(tokens.begin(), tokens.end(), str);
				int selected = i == tokens.end() ? 0 : i - tokens.begin();
				if (ImGui::ComboStl(p.name.c_str(), &selected, tokens)) {
					str = tokens[selected];
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Boolean: {
				core::String &str = _scriptParameters[i];
				bool checked = core::string::toBool(str);
				if (ImGui::Checkbox(p.name.c_str(), &checked)) {
					str = checked ? "1" : "0";
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Max:
				return;
			}
			ImGui::TooltipText("%s", p.description.c_str());
		}
	}

	if (ImGui::Button("New##scriptpanel")) {
		_scriptEditor = true;
		_activeScriptFilename = "";
		if (!_scripts.empty()) {
			const core::String &scriptName = _scripts[0].filename;
			ScriptBrush &scriptBrush = sceneMgr().modifier().scriptBrush();
			const core::String &script = scriptBrush.luaGenerator().load(scriptName);
			_textEditor.SetText(script);
		} else {
			_textEditor.SetText("");
		}
	}
	ImGui::TooltipText("Create a new lua script");
	if (validScriptIndex) {
		ImGui::SameLine();
		if (ImGui::Button("Edit##scriptpanel")) {
			_scriptEditor = true;
			_activeScriptFilename = _scripts[_currentScript].filename;
			_textEditor.SetText(_activeScript.c_str());
		}
		ImGui::TooltipText("Edit the selected lua script");
	}

	ImGui::URLButton(ICON_FA_BOOK " Scripting manual", "https://mgerhardy.github.io/vengi/voxedit/LUAScript/");
}

void BrushPanel::addShapes(command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();

	const ShapeType currentSelectedShapeType = modifier.shapeBrush().shapeType();
	if (ImGui::BeginCombo("Shape", ShapeTypeStr[(int)currentSelectedShapeType], ImGuiComboFlags_None)) {
		for (int i = 0; i < (int)ShapeType::Max; ++i) {
			const ShapeType type = (ShapeType)i;
			const bool selected = type == currentSelectedShapeType;
			if (ImGui::Selectable(ShapeTypeStr[i], selected)) {
				const core::String &typeStr = core::String::lower(ShapeTypeStr[i]);
				const core::String &cmd = "shape" + typeStr; // shapeaabb, ...
				command::executeCommands(cmd, &listener);
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

bool BrushPanel::mirrorAxisRadioButton(const char *title, math::Axis type,
									   command::CommandExecutionListener &listener) {
	voxedit::ModifierFacade &modifier = sceneMgr().modifier();
	ui::ScopedStyle style;
	veui::AxisStyleText(style, type, false);
	if (ImGui::RadioButton(title, modifier.shapeBrush().mirrorAxis() == type)) {
		core::String cmd = "mirroraxis"; // mirroraxisx, mirroraxisy, mirroraxisz
		cmd += math::getCharForAxis(type);
		command::executeCommands(cmd, &listener);
		return true;
	}
	return false;
}

void BrushPanel::addMirrorPlanes(command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();
	mirrorAxisRadioButton("Disable mirror##mirror", math::Axis::None, listener);
	ImGui::SameLine();
	mirrorAxisRadioButton("X##mirror", math::Axis::X, listener);
	ImGui::TooltipText("Mirror along the x axis at the reference position");
	ImGui::SameLine();
	mirrorAxisRadioButton("Y##mirror", math::Axis::Y, listener);
	ImGui::TooltipText("Mirror along the y axis at the reference position");
	ImGui::SameLine();
	mirrorAxisRadioButton("Z##mirror", math::Axis::Z, listener);
	ImGui::TooltipText("Mirror along the z axis at the reference position");
}

void BrushPanel::stampBrushUseSelection(scenegraph::SceneGraphNode &node, voxel::Palette &palette) {
	Modifier &modifier = sceneMgr().modifier();
	ui::ScopedStyle selectionStyle;
	if (modifier.selections().empty()) {
		selectionStyle.disableItem();
	}
	if (ImGui::Button("Use selection")) {
		const Selections &selections = modifier.selections();
		if (!selections.empty()) {
			core::ScopedPtr<voxel::RawVolume> copy(voxedit::tool::copy(node.volume(), selections));
			if (copy) {
				modifier.stampBrush().setVolume(*copy, palette);
			}
		}
	}
	ImGui::TooltipText("Use the current selection as new stamp");
}

void BrushPanel::stampBrushOptions(scenegraph::SceneGraphNode &node, voxel::Palette &palette, command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();
	StampBrush &brush = modifier.stampBrush();
	ImGui::InputTextWithHint("Model", "Select a model from the asset panel", &_stamp, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::ModelPayload)) {
			const core::String &filename = *(core::String *)payload->Data;
			if (brush.load(filename)) {
				_stamp = filename;
			}
		}
		ImGui::EndDragDropTarget();
	}

	bool center = brush.centerMode();
	if (ImGui::Checkbox("Center##modifiertype", &center)) {
		command::executeCommands("togglestampbrushcenter", &listener);
	}
	ImGui::TooltipCommand("togglestampbrushcenter");
	bool continuous = brush.continuousMode();
	if (ImGui::Checkbox("Continuous##modifiertype", &continuous)) {
		command::executeCommands("togglestampbrushcontinuous", &listener);
	}
	ImGui::TooltipCommand("togglestampbrushcontinuous");

	if (_stampPaletteIndex >= 0 && _stampPaletteIndex < palette.colorCount()) {
		const float size = 20;
		const ImVec2 v1 = ImGui::GetCursorScreenPos();
		const ImVec2 v2(v1.x + size, v1.y + size);
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(palette.color(_stampPaletteIndex)));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size);
	}
	ImGui::InputInt("##colorstampbrush", &_stampPaletteIndex, 0, 0, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::PaletteIndexPayload)) {
			_stampPaletteIndex = *(const uint8_t *)payload->Data;
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::SameLine();
	if (ImGui::Button("replace")) {
		brush.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, _stampPaletteIndex), palette);
	}
	ImGui::TooltipText("Replace all voxels in the stamp with the selected color");

	if (ImGui::CollapsingHeader("Reduce size")) {
		voxel::Region region = brush.volume()->region();
		glm::ivec3 size = region.getDimensionsInVoxels();
		if (ImGui::InputInt3("size##stampbrush", glm::value_ptr(size), ImGuiInputTextFlags_EnterReturnsTrue)) {
			if (glm::any(glm::greaterThan(size, region.getDimensionsInVoxels()))) {
				size = glm::min(size, region.getDimensionsInVoxels());
			}
			brush.setSize(size);
		}
	}
}

void BrushPanel::updatePlaneBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();
	if (modifier.modifierType() == ModifierType::Place) {
		ImGui::TextWrapped("Extrude voxels");
	} else if (modifier.modifierType() == ModifierType::Erase) {
		ImGui::TextWrapped("Erase voxels");
	} else if (modifier.modifierType() == ModifierType::Paint) {
		ImGui::TextWrapped("Paint voxels");
	} else if (modifier.modifierType() == (ModifierType::Paint | ModifierType::Erase)) {
		ImGui::TextWrapped("Override voxels");
	}
}

void BrushPanel::updateStampBrushPanel(command::CommandExecutionListener &listener) {
	const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const int nodeId = sceneGraph.activeNode();
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	voxel::Palette &palette = node.palette();

	Modifier &modifier = sceneMgr().modifier();
	if (!modifier.stampBrush().active()) {
		ImGui::TextWrapped("Select a model from the asset panel");
		ui::ScopedStyle style;
		style.disableItem();
		stampBrushOptions(node, palette, listener);
	} else {
		stampBrushOptions(node, palette, listener);
	}
	stampBrushUseSelection(node, palette);
	if (ImGui::Button("Convert palette")) {
		modifier.stampBrush().convertToPalette(palette);
	}
}

void BrushPanel::updateShapeBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();
	addShapes(listener);
	addMirrorPlanes(listener);
	bool center = modifier.shapeBrush().centerMode();
	if (ImGui::Checkbox("Center##modifiertype", &center)) {
		command::executeCommands("toggleshapebrushcenter", &listener);
	}
	ImGui::TooltipCommand("toggleshapebrushcenter");
	bool single = modifier.shapeBrush().singleMode();
	if (ImGui::Checkbox("Single##modifiertype", &single)) {
		command::executeCommands("toggleshapebrushsingle", &listener);
	}
	ImGui::TooltipCommand("toggleshapebrushsingle");
}

void BrushPanel::brushRegion() {
	Modifier &modifier = sceneMgr().modifier();
	ShapeBrush *brush = modifier.activeShapeBrush();
	if (brush == nullptr) {
		return;
	}
	voxel::Region region = modifier.calcBrushRegion();
	if (ImGui::CollapsingHeader("Brush region")) {
		ui::ScopedStyle style;
		if (brush->singleMode()) {
			style.disableItem();
		}
		glm::ivec3 mins = region.getLowerCorner();
		glm::ivec3 maxs = region.getUpperCorner();
		if (ImGui::InputInt3("mins##regionbrush", glm::value_ptr(mins))) {
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		if (ImGui::InputInt3("maxs##regionbrush", glm::value_ptr(maxs))) {
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		if (ImGui::Button(ICON_FK_ARROW_RIGHT)) {
			_face = voxel::FaceNames::PositiveX;
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		ImGui::TooltipText("Generate in positive x direction");
		ImGui::SameLine();
		if (ImGui::Button(ICON_FK_ARROW_LEFT)) {
			_face = voxel::FaceNames::NegativeX;
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		ImGui::TooltipText("Generate in negative x direction");
		ImGui::SameLine();
		if (ImGui::Button(ICON_FK_ARROW_UP)) {
			_face = voxel::FaceNames::PositiveY;
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		ImGui::TooltipText("Generate in positive y direction");
		ImGui::SameLine();
		if (ImGui::Button(ICON_FK_ARROW_DOWN)) {
			_face = voxel::FaceNames::NegativeY;
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		ImGui::TooltipText("Generate in negative y direction");
	}
}

void BrushPanel::brushSettings(command::CommandExecutionListener &listener) {
	const Modifier &modifier = sceneMgr().modifier();
	if (ImGui::CollapsingHeader("Brush settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (modifier.brushType() == BrushType::Script) {
			updateScriptBrushPanel(listener);
		} else if (modifier.brushType() == BrushType::Shape) {
			updateShapeBrushPanel(listener);
		} else if (modifier.brushType() == BrushType::Stamp) {
			updateStampBrushPanel(listener);
		} else if (modifier.brushType() == BrushType::Plane) {
			updatePlaneBrushPanel(listener);
		}
	}
}

void BrushPanel::addModifiers(command::CommandExecutionListener &listener) {
	ui::ScopedStyle style;
	style.setFont(imguiApp()->bigIconFont());
	const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
	ui::Toolbar toolbar(buttonSize, &listener);
	const voxedit::ModifierFacade &modifier = sceneMgr().modifier();
	const bool moverride = modifier.isMode(ModifierType::Place | ModifierType::Erase);
	const bool mplace = !moverride && modifier.isMode(ModifierType::Place);
	const bool merase = !moverride && modifier.isMode(ModifierType::Erase);

	toolbar.button(ICON_FA_CUBE, "actionplace", !mplace);
	toolbar.button(ICON_FA_ERASER, "actionerase", !merase);
	toolbar.button(ICON_FA_DIAGRAM_NEXT, "actionoverride", !moverride);
	toolbar.button(ICON_FA_PAINTBRUSH, "actionpaint", !modifier.isMode(ModifierType::Paint));
	toolbar.button(ICON_FA_EXPAND, "actionselect", !modifier.isMode(ModifierType::Select));
	toolbar.button(ICON_FA_ELLIPSIS, "actionpath", !modifier.isMode(ModifierType::Path));
	toolbar.button(ICON_FA_ELLIPSIS, "actionline", !modifier.isMode(ModifierType::Line));
	toolbar.button(ICON_FA_EYE_DROPPER, "actioncolorpicker", !modifier.isMode(ModifierType::ColorPicker));
}

void BrushPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		addModifiers(listener);
	}

	Modifier &modifier = sceneMgr().modifier();
	if (modifier.isMode(ModifierType::ColorPicker)) {
		ImGui::TextWrapped("Click on a voxel to pick the color");
	} else if (modifier.isMode(ModifierType::Line)) {
		ImGui::TextWrapped("Draws a line from the reference position to the current cursor position");
	} else if (modifier.isMode(ModifierType::Path)) {
		ImGui::TextWrapped("Draws a path over existing voxels");
	} else if (modifier.isMode(ModifierType::Select)) {
		ImGui::TextWrapped("Select areas of voxels");
	} else {
		const int currentBrush = (int)modifier.brushType();
		const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
		ui::Toolbar toolbar(buttonSize, &listener);
		for (int i = 0; i < (int)BrushType::Max; ++i) {
			auto func = [&modifier, i]() { modifier.setBrushType((BrushType)i); };
			toolbar.button(BrushTypeIcons[i], BrushTypeStr[i], func, currentBrush != i);
		}
		toolbar.end();

		brushRegion();
		brushSettings(listener);

		ImGui::Separator();
	}
	ImGui::End();
}

bool BrushPanel::updateEditor(const char *title, ui::IMGUIApp *app) {
	if (!_scriptEditor) {
		return false;
	}
	if (ImGui::Begin(title, &_scriptEditor, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar)) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu(ICON_FA_FILE " File##scripteditor")) {
				if (ImGui::MenuItem(ICON_FA_CHECK " Apply and execute##scripteditor")) {
					_activeScript = _textEditor.GetText();
					reloadScriptParameters(_activeScript);
				}
				if (!_activeScriptFilename.empty()) {
					if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save##scripteditor")) {
						if (app->filesystem()->write(core::string::path("scripts", _activeScriptFilename),
													 _textEditor.GetText())) {
							_activeScript = _textEditor.GetText();
							reloadScriptParameters(_activeScript);
						}
					}
					ImGui::TooltipText("Overwrite scripts/%s", _activeScriptFilename.c_str());
				}
				if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save As##scripteditor")) {
					core::Var::getSafe(cfg::UILastDirectory)->setVal("scripts/");
					app->saveDialog(
						[&](const core::String &file, const io::FormatDescription *desc) {
							if (app->filesystem()->write(file, _textEditor.GetText())) {
								_scripts.clear();
								_currentScript = -1;
								Log::info("Saved script to %s", file.c_str());
							} else {
								Log::warn("Failed to save script %s", file.c_str());
							}
						},
						{}, io::format::lua());
				}
				if (ImGui::MenuItem("Close##scripteditor")) {
					_scriptEditor = false;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu(ICON_FK_PENCIL " Edit##scripteditor")) {
				if (ImGui::MenuItem(ICON_FA_ROTATE_LEFT " Undo##scripteditor", nullptr, nullptr,
									_textEditor.CanUndo())) {
					_textEditor.Undo();
				}
				if (ImGui::MenuItem(ICON_FA_ROTATE_RIGHT " Redo##scripteditor", nullptr, nullptr,
									_textEditor.CanRedo())) {
					_textEditor.Redo();
				}

				ImGui::Separator();

				if (ImGui::MenuItem(ICON_FA_COPY " Copy##scripteditor", nullptr, nullptr, _textEditor.HasSelection())) {
					_textEditor.Copy();
				}
				if (ImGui::MenuItem(ICON_FA_SCISSORS " Cut##scripteditor", nullptr, nullptr,
									_textEditor.HasSelection())) {
					_textEditor.Cut();
				}
				if (ImGui::MenuItem("Delete##scripteditor", nullptr, nullptr, _textEditor.HasSelection())) {
					_textEditor.Delete();
				}
				if (ImGui::MenuItem(ICON_FA_PASTE " Paste##scripteditor", nullptr, nullptr,
									ImGui::GetClipboardText() != nullptr)) {
					_textEditor.Paste();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Select all##scripteditor", nullptr, nullptr)) {
					_textEditor.SetSelection(TextEditor::Coordinates(),
											 TextEditor::Coordinates(_textEditor.GetTotalLines(), 0));
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		_textEditor.Render(title);
	}
	ImGui::End();
	return true;
}

} // namespace voxedit
