/**
 * @file
 */

#include "OptionsPanel.h"
#include "voxedit-util/SceneManager.h"
#include "MenuBar.h"
#include "ViewMode.h"
#include "core/ConfigVar.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Array.h"
#include "core/collection/DynamicArray.h"
#include "ui/ScopedID.h"
#include "imgui.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/PopupAbout.h"
#include "voxedit-util/Config.h"
#include "voxel/SurfaceExtractor.h"

namespace voxedit {

bool OptionsPanel::matchesFilter(const char *text) const {
	if (_filter.empty()) {
		return true;
	}
	return core::string::icontains(text, _filter);
}

bool OptionsPanel::matchesVarFilter(const char *varName) const {
	const core::VarPtr &var = core::getVar(varName);
	const char *title = var->title().empty() ? varName : _(var->title().c_str());
	return matchesFilter(title);
}

bool OptionsPanel::hasFilter() const {
	return !_filter.empty();
}

bool OptionsPanel::categoryHasMatch(OptionCategory category) const {
	switch (category) {
	case OptionCategory::UserInterface:
		return matchesVarFilter(cfg::UIFontSize) || matchesVarFilter(cfg::UIStyle) ||
			   matchesVarFilter(cfg::CoreLanguage) || matchesVarFilter(cfg::UIMultiMonitor) ||
			   matchesVarFilter(cfg::UINotifyDismissMillis) || matchesVarFilter(cfg::VoxEditTipOftheDay);
	case OptionCategory::Editor:
		return matchesVarFilter(cfg::VoxEditViewMode) || matchesVarFilter(cfg::VoxEditShowColorPicker) ||
			   matchesVarFilter(cfg::VoxEditColorWheel) || matchesVarFilter(cfg::VoxEditAnimationSpeed) ||
			   matchesVarFilter(cfg::VoxEditAutoSaveSeconds) || matchesVarFilter(cfg::VoxEditViewports) ||
			   matchesVarFilter(cfg::ClientCameraZoomSpeed) || matchesVarFilter(cfg::VoxEditViewdistance) ||
			   matchesVarFilter(cfg::CoreColorReduction) || matchesVarFilter(cfg::VoxRenderMeshMode);
	case OptionCategory::Metrics:
		return matchesVarFilter(cfg::MetricFlavor);
	case OptionCategory::Layout:
		return matchesFilter(_("Reset layout"));
	case OptionCategory::Display:
		return matchesVarFilter(cfg::VoxEditShowgrid) || matchesVarFilter(cfg::VoxEditShowaxis) ||
			   matchesVarFilter(cfg::VoxEditShowlockedaxis) || matchesVarFilter(cfg::VoxEditShowaabb) ||
			   matchesVarFilter(cfg::VoxEditShowBones) || matchesVarFilter(cfg::VoxEditShowPlane) ||
			   matchesVarFilter(cfg::VoxEditPlaneSize);
	case OptionCategory::Rendering:
		return matchesVarFilter(cfg::RenderOutline) || matchesVarFilter(cfg::RenderNormals) ||
			   matchesVarFilter(cfg::RenderCheckerBoard) || matchesVarFilter(cfg::VoxEditShadingMode) ||
			   matchesVarFilter(cfg::ClientBloom) || matchesVarFilter(cfg::ToneMapping);
	case OptionCategory::Renderer:
		return matchesVarFilter(cfg::ClientShadowMapSize) || matchesVarFilter(cfg::ClientGamma) ||
			   matchesVarFilter(cfg::ClientVSync);
	case OptionCategory::MeshExport:
		return matchesVarFilter(cfg::VoxformatMergequads) || matchesVarFilter(cfg::VoxformatReusevertices) ||
			   matchesVarFilter(cfg::VoxformatAmbientocclusion) || matchesVarFilter(cfg::VoxformatQuads) ||
			   matchesVarFilter(cfg::VoxformatWithColor) || matchesVarFilter(cfg::VoxformatWithNormals) ||
			   matchesVarFilter(cfg::VoxformatWithtexcoords) || matchesVarFilter(cfg::VoxformatTransform) ||
			   matchesVarFilter(cfg::VoxformatOptimize);
	case OptionCategory::VoxelImportExport:
		return matchesVarFilter(cfg::VoxelCreatePalette) || matchesVarFilter(cfg::VoxformatRGBFlattenFactor) ||
			   matchesVarFilter(cfg::VoxformatRGBWeightedAverage) || matchesVarFilter(cfg::VoxformatSaveVisibleOnly) ||
			   matchesVarFilter(cfg::VoxformatMerge) || matchesVarFilter(cfg::VoxformatScale) ||
			   matchesVarFilter(cfg::VoxformatFillHollow);
	case OptionCategory::AllVariables: {
		bool found = false;
		core::Var::visit([&](const core::VarPtr &var) {
			if (found) {
				return;
			}
			const bool matchName = core::string::icontains(var->name(), _filter);
			const bool matchTitle = core::string::icontains(_(var->title().c_str()), _filter);
			const bool matchDescription = core::string::icontains(_(var->description().c_str()), _filter);
			if (matchName || matchTitle || matchDescription) {
				found = true;
			}
		});
		return found;
	}
	case OptionCategory::Max:
		break;
	}
	return false;
}

void OptionsPanel::renderUserInterface() {
	if (matchesVarFilter(cfg::UIFontSize)) {
		ImGui::InputVarInt(cfg::UIFontSize, 1, 5);
	}
	if (matchesVarFilter(cfg::UIStyle)) {
		const core::VarPtr &uiStyleVar = core::getVar(cfg::UIStyle);
		int currentStyle = uiStyleVar->intVal();
		const core::String label = _(uiStyleVar->title().c_str());
		if (ImGui::BeginCombo(label.c_str(), ImGui::GetStyleName(currentStyle))) {
			for (int i = 0; i < ImGui::MaxStyles; ++i) {
				const bool isSelected = (currentStyle == i);
				if (ImGui::Selectable(ImGui::GetStyleName(i), isSelected)) {
					uiStyleVar->setVal(i);
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
	if (matchesVarFilter(cfg::CoreLanguage)) {
		_app->languageOption();
	}
	if (matchesVarFilter(cfg::UIMultiMonitor)) {
		ImGui::IconCheckboxVar(ICON_LC_TV_MINIMAL, cfg::UIMultiMonitor);
	}
	if (matchesVarFilter(cfg::UINotifyDismissMillis)) {
		ImGui::InputVarFloat(cfg::UINotifyDismissMillis);
	}
	if (matchesVarFilter(cfg::VoxEditTipOftheDay)) {
		ImGui::IconCheckboxVar(ICON_LC_LIGHTBULB, cfg::VoxEditTipOftheDay);
	}
}

void OptionsPanel::renderEditor() {
	if (matchesVarFilter(cfg::VoxEditViewMode)) {
		MenuBar::viewModeOption();
	}
	if (matchesVarFilter(cfg::VoxEditShowColorPicker)) {
		ImGui::CheckboxVar(cfg::VoxEditShowColorPicker);
	}
	if (matchesVarFilter(cfg::VoxEditColorWheel)) {
		ImGui::CheckboxVar(cfg::VoxEditColorWheel);
	}
	if (matchesVarFilter(cfg::VoxEditAnimationSpeed)) {
		ImGui::InputVarInt(cfg::VoxEditAnimationSpeed);
	}
	if (matchesVarFilter(cfg::VoxEditAutoSaveSeconds)) {
		ImGui::InputVarInt(cfg::VoxEditAutoSaveSeconds);
	}
	if (matchesVarFilter(cfg::VoxEditViewports)) {
		ImGui::InputVarInt(cfg::VoxEditViewports, 1, 1);
	}
	if (matchesVarFilter(cfg::ClientCameraZoomSpeed)) {
		ImGui::SliderVarFloat(cfg::ClientCameraZoomSpeed);
	}
	if (matchesVarFilter(cfg::VoxEditViewdistance)) {
		ImGui::SliderVarInt(cfg::VoxEditViewdistance);
	}
	if (matchesVarFilter(cfg::CoreColorReduction)) {
		_app->colorReductionOptions();
	}
	if (matchesVarFilter(cfg::VoxRenderMeshMode)) {
		static const core::Array<core::String, (int)voxel::SurfaceExtractionType::Binary + 1> meshModes = {
			_("Cubes"), _("Marching cubes"), _("Binary")};
		ImGui::ComboVar(cfg::VoxRenderMeshMode, meshModes);
	}
}

void OptionsPanel::renderMetrics() {
	if (matchesVarFilter(cfg::MetricFlavor)) {
		ui::metricOption();
	}
}

void OptionsPanel::renderLayout() {
	if (matchesFilter(_("Reset layout"))) {
		if (ImGui::ButtonFullWidth(_("Reset layout"))) {
			_resetDockLayout = true;
		}
	}
}

void OptionsPanel::renderDisplay() {
	if (matchesVarFilter(cfg::VoxEditShowgrid)) {
		ImGui::IconCheckboxVar(ICON_LC_GRID_3X3, cfg::VoxEditShowgrid);
	}
	if (matchesVarFilter(cfg::VoxEditShowaxis)) {
		ImGui::IconCheckboxVar(ICON_LC_ROTATE_3D, cfg::VoxEditShowaxis);
	}
	if (matchesVarFilter(cfg::VoxEditShowlockedaxis)) {
		ImGui::IconCheckboxVar(ICON_LC_LOCK, cfg::VoxEditShowlockedaxis);
	}
	if (matchesVarFilter(cfg::VoxEditShowaabb)) {
		ImGui::IconCheckboxVar(ICON_LC_BOX, cfg::VoxEditShowaabb);
	}
	if (matchesVarFilter(cfg::VoxEditShowBones)) {
		ImGui::IconCheckboxVar(ICON_LC_BONE, cfg::VoxEditShowBones);
	}
	if (matchesVarFilter(cfg::VoxEditShowPlane)) {
		ImGui::IconCheckboxVar(ICON_LC_FRAME, cfg::VoxEditShowPlane);
	}
	if (matchesVarFilter(cfg::VoxEditPlaneSize)) {
		ImGui::IconSliderVarInt(ICON_LC_GRIP, cfg::VoxEditPlaneSize);
	}
}

void OptionsPanel::renderRendering() {
	const bool isMarchingCubes =
		core::getVar(cfg::VoxRenderMeshMode)->intVal() == (int)voxel::SurfaceExtractionType::MarchingCubes;
	ImGui::BeginDisabled(isMarchingCubes);
	if (matchesVarFilter(cfg::RenderOutline)) {
		ImGui::IconCheckboxVar(ICON_LC_BOX, cfg::RenderOutline);
	}
	if (matchesVarFilter(cfg::RenderNormals)) {
		if (viewModeNormalPalette(core::getVar(cfg::VoxEditViewMode)->intVal())) {
			ImGui::IconCheckboxVar(ICON_LC_BOX, cfg::RenderNormals);
		}
	}
	if (matchesVarFilter(cfg::RenderCheckerBoard)) {
		ImGui::IconCheckboxVar(ICON_LC_BRICK_WALL, cfg::RenderCheckerBoard);
	}
	ImGui::EndDisabled();

	if (matchesVarFilter(cfg::VoxEditShadingMode)) {
		const core::VarPtr &shadingVar = core::getVar(cfg::VoxEditShadingMode);
		const char *shadingModeLabels[] = {_("Unlit"), _("Lit"), _("Shadows")};
		int currentShadingMode = shadingVar->intVal();
		const char *currentLabel = (currentShadingMode >= 0 && currentShadingMode < (int)lengthof(shadingModeLabels))
									   ? shadingModeLabels[currentShadingMode]
									   : _("Unknown");

		if (ImGui::BeginIconCombo(ICON_LC_SPOTLIGHT, _(shadingVar->title().c_str()), currentLabel)) {
			for (int i = 0; i < (int)lengthof(shadingModeLabels); ++i) {
				const bool isSelected = (currentShadingMode == i);
				if (ImGui::Selectable(shadingModeLabels[i], isSelected)) {
					shadingVar->setVal(i);
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
	if (matchesVarFilter(cfg::ClientBloom)) {
		ImGui::IconCheckboxVar(ICON_LC_SUN, cfg::ClientBloom);
	}
	if (matchesVarFilter(cfg::ToneMapping)) {
		ImGui::IconSliderVarInt(ICON_LC_ECLIPSE, cfg::ToneMapping);
	}
}

void OptionsPanel::renderRenderer() {
	if (matchesVarFilter(cfg::ClientShadowMapSize)) {
		ImGui::InputVarInt(cfg::ClientShadowMapSize);
	}
	if (matchesVarFilter(cfg::ClientGamma)) {
		ImGui::SliderVarFloat(cfg::ClientGamma);
	}
	if (matchesVarFilter(cfg::ClientVSync)) {
		ImGui::CheckboxVar(cfg::ClientVSync);
	}
}

void OptionsPanel::renderMeshExport() {
	if (matchesVarFilter(cfg::VoxformatMergequads)) {
		ImGui::CheckboxVar(cfg::VoxformatMergequads);
	}
	if (matchesVarFilter(cfg::VoxformatReusevertices)) {
		ImGui::CheckboxVar(cfg::VoxformatReusevertices);
	}
	if (matchesVarFilter(cfg::VoxformatAmbientocclusion)) {
		ImGui::CheckboxVar(cfg::VoxformatAmbientocclusion);
	}
	if (matchesVarFilter(cfg::VoxformatQuads)) {
		ImGui::CheckboxVar(cfg::VoxformatQuads);
	}
	if (matchesVarFilter(cfg::VoxformatWithColor)) {
		ImGui::CheckboxVar(cfg::VoxformatWithColor);
	}
	if (matchesVarFilter(cfg::VoxformatWithNormals)) {
		ImGui::CheckboxVar(cfg::VoxformatWithNormals);
	}
	if (matchesVarFilter(cfg::VoxformatWithtexcoords)) {
		ImGui::CheckboxVar(cfg::VoxformatWithtexcoords);
	}
	if (matchesVarFilter(cfg::VoxformatTransform)) {
		ImGui::CheckboxVar(cfg::VoxformatTransform);
	}
	if (matchesVarFilter(cfg::VoxformatOptimize)) {
		ImGui::CheckboxVar(cfg::VoxformatOptimize);
	}
}

void OptionsPanel::renderVoxelImportExport() {
	if (matchesVarFilter(cfg::VoxelCreatePalette)) {
		ImGui::CheckboxVar(cfg::VoxelCreatePalette);
	}
	if (matchesVarFilter(cfg::VoxformatRGBFlattenFactor)) {
		ImGui::InputVarFloat(cfg::VoxformatRGBFlattenFactor);
	}
	if (matchesVarFilter(cfg::VoxformatRGBWeightedAverage)) {
		ImGui::CheckboxVar(cfg::VoxformatRGBWeightedAverage);
	}
	if (matchesVarFilter(cfg::VoxformatSaveVisibleOnly)) {
		ImGui::CheckboxVar(cfg::VoxformatSaveVisibleOnly);
	}
	if (matchesVarFilter(cfg::VoxformatMerge)) {
		ImGui::CheckboxVar(cfg::VoxformatMerge);
	}
	if (matchesVarFilter(cfg::VoxformatScale)) {
		ImGui::InputVarFloat(cfg::VoxformatScale);
	}
	if (matchesVarFilter(cfg::VoxformatFillHollow)) {
		ImGui::CheckboxVar(cfg::VoxformatFillHollow);
	}
}

void OptionsPanel::renderAllVariables() {
	static const uint32_t TableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
									   ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInner |
									   ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
	const ImVec2 outerSize(0, 0);
	if (ImGui::BeginTable("##cvars", 4, TableFlags, outerSize)) {
		ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn(_("Value"), ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("##reset", ImGuiTableFlags_SizingFixedFit);
		ImGui::TableSetupColumn(_("Description"), ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		core::DynamicArray<core::Var *> vars;
		vars.reserve(core::Var::size());
		core::Var::visit([&](const core::VarPtr &var) {
			vars.push_back(var.get());
		});

		core::DynamicArray<core::Var *> filteredVars;
		if (hasFilter()) {
			for (core::Var *var : vars) {
				const bool matchName = core::string::icontains(var->name(), _filter);
				const bool matchValue = core::string::icontains(var->strVal(), _filter);
				const bool matchTitle = core::string::icontains(_(var->title().c_str()), _filter);
				const bool matchDescription = core::string::icontains(_(var->description().c_str()), _filter);
				if (matchName || matchValue || matchTitle || matchDescription) {
					filteredVars.push_back(var);
				}
			}
		} else {
			filteredVars = vars;
		}

		ImGuiListClipper clipper;
		clipper.Begin((int)filteredVars.size());
		while (clipper.Step()) {
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
				core::Var *var = filteredVars[i];
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(var->name().c_str());
				ImGui::TableNextColumn();
				const bool readOnly = (var->getFlags() & core::CV_READONLY) != 0;
				ImGui::BeginDisabled(readOnly);
				const core::String type = "##" + var->name();
				if (var->type() == core::VarType::Bool) {
					bool value = var->boolVal();
					if (ImGui::Checkbox(type.c_str(), &value)) {
						var->setVal(value);
					}
				} else {
					int flags = 0;
					const bool secret = (var->getFlags() & core::CV_SECRET) != 0;
					if (secret) {
						flags |= ImGuiInputTextFlags_Password;
					}
					core::String value = var->strVal();
					if (ImGui::InputText(type.c_str(), &value, flags)) {
						var->setVal(value);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextColumn();
				if (!readOnly) {
					ui::ScopedID id(var->name());
					if (ImGui::Button(_("Reset"))) {
						var->reset();
					}
					ImGui::TooltipTextUnformatted(_("Reset to default value"));
				}
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(var->description().c_str());
			}
		}
		ImGui::EndTable();
	}
}

void OptionsPanel::renderContent() {
	switch (_selectedCategory) {
	case OptionCategory::UserInterface:
		renderUserInterface();
		break;
	case OptionCategory::Editor:
		renderEditor();
		break;
	case OptionCategory::Metrics:
		renderMetrics();
		break;
	case OptionCategory::Layout:
		renderLayout();
		break;
	case OptionCategory::Display:
		renderDisplay();
		break;
	case OptionCategory::Rendering:
		renderRendering();
		break;
	case OptionCategory::Renderer:
		renderRenderer();
		break;
	case OptionCategory::MeshExport:
		renderMeshExport();
		break;
	case OptionCategory::VoxelImportExport:
		renderVoxelImportExport();
		break;
	case OptionCategory::AllVariables:
		renderAllVariables();
		break;
	case OptionCategory::Max:
		break;
	}
}

struct TreeGroup {
	const char *icon;
	const char *label;
	struct {
		OptionCategory category;
		const char *label;
	} children[4];
	int childCount;
};

void OptionsPanel::renderTree() {
	const bool filtering = hasFilter();

	const TreeGroup groups[] = {
		{ICON_LC_SETTINGS, _("General"),
		 {{OptionCategory::UserInterface, _("User Interface")},
		  {OptionCategory::Editor, _("Editor")},
		  {OptionCategory::Metrics, _("Metrics")},
		  {OptionCategory::Layout, _("Layout")}},
		 4},
		{ICON_LC_MONITOR, _("Viewport"),
		 {{OptionCategory::Display, _("Display")},
		  {OptionCategory::Rendering, _("Rendering")},
		  {}, {}},
		 2},
		{ICON_LC_PAINTBRUSH, _("Renderer"),
		 {{OptionCategory::Renderer, _("Renderer")},
		  {}, {}, {}},
		 1},
		{ICON_LC_FILE_OUTPUT, _("Format"),
		 {{OptionCategory::MeshExport, _("Mesh Export")},
		  {OptionCategory::VoxelImportExport, _("Voxel Import/Export")},
		  {}, {}},
		 2},
		{ICON_LC_CODE, _("Advanced"),
		 {{OptionCategory::AllVariables, _("All Variables")},
		  {}, {}, {}},
		 1},
	};

	const bool filterChanged = filtering && (_filter != _lastFilter);
	bool firstMatch = filterChanged;
	for (int g = 0; g < (int)lengthof(groups); ++g) {
		const TreeGroup &group = groups[g];

		// check if any child in this group has matching items
		bool groupHasMatch = false;
		if (filtering) {
			for (int c = 0; c < group.childCount; ++c) {
				if (categoryHasMatch(group.children[c].category)) {
					groupHasMatch = true;
					break;
				}
			}
			if (!groupHasMatch) {
				continue;
			}
		}

		if (filtering) {
			ImGui::SetNextItemOpen(true);
		}
		const core::String groupLabel = core::String::format("%s %s", group.icon, group.label);
		const ImGuiTreeNodeFlags groupFlags = ImGuiTreeNodeFlags_DefaultOpen;
		if (ImGui::TreeNodeEx(groupLabel.c_str(), groupFlags)) {
			for (int c = 0; c < group.childCount; ++c) {
				const OptionCategory cat = group.children[c].category;
				const char *childLabel = group.children[c].label;

				if (filtering && !categoryHasMatch(cat)) {
					continue;
				}

				if (filtering && firstMatch) {
					_selectedCategory = cat;
					firstMatch = false;
				}

				ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				if (_selectedCategory == cat) {
					leafFlags |= ImGuiTreeNodeFlags_Selected;
				}
				ImGui::TreeNodeEx(childLabel, leafFlags);
				if (ImGui::IsItemClicked()) {
					_selectedCategory = cat;
				}
			}
			ImGui::TreePop();
		}
	}
}

bool OptionsPanel::shouldResetDockLayout() {
	if (_resetDockLayout) {
		_resetDockLayout = false;
		return true;
	}
	return false;
}

void OptionsPanel::update(const char *id) {
	if (!_visible) {
		return;
	}
	core_trace_scoped(OptionsPanel);
	const core::String title = makeTitle(ICON_LC_SETTINGS, _("Options"), id);
	if (_requestFocus) {
		ImGui::SetNextWindowFocus();
		_requestFocus = false;
	}
	if (ImGui::Begin(title.c_str(), &_visible, ImGuiWindowFlags_NoFocusOnAppearing)) {
		ImGui::InputTextWithHint(_("Search"), ICON_LC_SEARCH, &_filter);
		ImGui::Separator();

		const float treeWidth = ImGui::GetFontSize() * 12.0f;

		if (ImGui::BeginChild("##optionstree", ImVec2(treeWidth, 0), ImGuiChildFlags_ResizeX)) {
			renderTree();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		if (ImGui::BeginChild("##optionscontent", ImVec2(0, 0))) {
			renderContent();
		}
		ImGui::EndChild();

		_lastFilter = _filter;
	}
	ImGui::End();
}

} // namespace voxedit
