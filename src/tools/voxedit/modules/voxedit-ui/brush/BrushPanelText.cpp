/**
 * @file
 */

#include "BrushPanelText.h"
#include "BrushPanelWidgets.h"
#include "app/I18N.h"
#include "io/Filesystem.h"
#include "ui/IMGUIEx.h"
#include "ui/font/FontResolver.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/TextBrush.h"

namespace voxedit {

static int findFontIndex(const core::DynamicArray<core::String> &paths, const core::String &font) {
	for (int i = 0; i < (int)paths.size(); ++i) {
		if (paths[i] == font) {
			return i;
		}
	}
	const core::String base = core::string::extractFilenameWithExtension(font);
	for (int i = 0; i < (int)paths.size(); ++i) {
		if (core::string::extractFilenameWithExtension(paths[i]) == base) {
			return i;
		}
	}
	return -1;
}

void BrushPanelText::loadFontList() {
	if (_fontsLoaded) {
		return;
	}
	_fontsLoaded = true;
	_fontPaths.clear();
	_fontLabels.clear();

	auto addFont = [&](const core::String &path) {
		for (const core::String &existing : _fontPaths) {
			if (existing == path) {
				return;
			}
		}
		_fontPaths.push_back(path);
		_fontLabels.push_back(core::string::extractFilenameWithExtension(path));
	};

	const io::FilePtr defaultFont = io::filesystem()->open("font.ttf");
	if (defaultFont->exists()) {
		addFont("font.ttf");
	}

	core::DynamicArray<io::FilesystemEntry> entities;
	static const char *fontFilters[] = {"*.ttf", "*.otf", "*.ttc"};
	for (int i = 0; i < lengthof(fontFilters); ++i) {
		io::filesystem()->list("font", entities, fontFilters[i]);
	}
	for (const io::FilesystemEntry &entry : entities) {
		addFont(io::filesystem()->filePath(entry.fullPath));
	}

	const core::DynamicArray<core::String> systemFonts = ui::font::findSystemFonts();
	for (const core::String &path : systemFonts) {
		addFont(path);
	}

	for (int i = 0; i < (int)_fontLabels.size(); ++i) {
		int dupes = 0;
		for (int j = 0; j < (int)_fontLabels.size(); ++j) {
			if (_fontLabels[j] == _fontLabels[i]) {
				++dupes;
			}
		}
		if (dupes > 1) {
			const core::String parentDir = core::string::extractDir(_fontPaths[i]);
			_fontLabels[i] = core::String::format("%s (%s)", _fontLabels[i].c_str(),
												  core::string::extractFilename(parentDir).c_str());
		}
	}

	core::DynamicArray<int> order;
	order.reserve(_fontPaths.size());
	for (int i = 0; i < (int)_fontPaths.size(); ++i) {
		order.push_back(i);
	}
	order.sort([&](int a, int b) { return _fontLabels[a] < _fontLabels[b]; });

	core::DynamicArray<core::String> sortedPaths;
	core::DynamicArray<core::String> sortedLabels;
	sortedPaths.reserve(_fontPaths.size());
	sortedLabels.reserve(_fontLabels.size());
	for (int i = 0; i < (int)order.size(); ++i) {
		const int idx = order[i];
		sortedPaths.push_back(_fontPaths[idx]);
		sortedLabels.push_back(_fontLabels[idx]);
	}
	_fontPaths = sortedPaths;
	_fontLabels = sortedLabels;
}

void BrushPanelText::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	TextBrush &brush = modifier.textBrush();
	if (ImGui::InputText(_("Text"), &brush.input())) {
		brush.markDirty();
	}

	ImGui::SetNextItemWidth(ImGui::Size(10.0f));
	int size = brush.size();
	if (ImGui::InputInt(ICON_LC_MOVE_VERTICAL, &size)) {
		brush.setSize(size);
	}
	ImGui::TooltipTextUnformatted(_("Font size"));
	ImGui::SameLine();

	ImGui::SetNextItemWidth(ImGui::Size(10.0f));
	int spacing = brush.spacing();
	if (ImGui::InputInt(ICON_LC_MOVE_HORIZONTAL "##textinput", &spacing)) {
		brush.setSpacing(spacing);
	}
	ImGui::TooltipTextUnformatted(_("Horizontal spacing"));

	int thickness = brush.thickness();
	if (ImGui::InputInt(ICON_LC_EXPAND "##textinput", &thickness)) {
		brush.setThickness(thickness);
	}
	ImGui::TooltipTextUnformatted(_("Thickness"));

	const float buttonWidth = ImGui::GetFontSize() * 4.0f;
	ImGui::AxisCommandButton(math::Axis::X, _("X"), "textbrushaxis x", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
	ImGui::SameLine();
	ImGui::AxisCommandButton(math::Axis::Y, _("Y"), "textbrushaxis y", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
	ImGui::SameLine();
	ImGui::AxisCommandButton(math::Axis::Z, _("Z"), "textbrushaxis z", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);

	brushpanel::addMirrorPlanes(listener, modifier.textBrush());
	ImGui::Separator();
	brushpanel::addBrushClampingOption(brush);

	loadFontList();

	int currentFont = findFontIndex(_fontPaths, brush.font());
	if (currentFont < 0 && !brush.font().empty()) {
		_fontPaths.push_front(brush.font());
		_fontLabels.push_front(core::string::extractFilenameWithExtension(brush.font()));
		currentFont = 0;
	} else if (currentFont < 0 && !_fontPaths.empty()) {
		currentFont = 0;
	}

	if (!_fontPaths.empty() && ImGui::SearchableComboItems(_("Font"), &currentFont, _fontLabels, _fontSearchFilter)) {
		brush.setFont(_fontPaths[currentFont]);
		brush.markDirty();
	}
	if (currentFont >= 0 && currentFont < (int)_fontPaths.size() &&
		ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(_("Font to use for rendering text"));
		ImGui::Separator();
		ImGui::TextUnformatted(_fontPaths[currentFont].c_str());
		ImGui::EndTooltip();
	}
}

} // namespace voxedit
