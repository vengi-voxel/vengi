/**
 * @file
 */

#include "CollectionPanel.h"
#include "core/StringUtil.h"
#include "ui/IMGUIEx.h"
#include "voxelcollection/Downloader.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelcollection {

CollectionPanel::CollectionPanel(ui::IMGUIApp *app, const video::TexturePoolPtr &texturePool)
	: Super(app), _texturePool(texturePool) {
}

CollectionPanel::~CollectionPanel() {
}

bool CollectionPanel::filtered(const voxelcollection::VoxelFile &voxelFile) const {
	if (!_currentFilterName.empty() && !core::string::icontains(voxelFile.name, _currentFilterName)) {
		return true;
	}
	if (!_currentFilterLicense.empty() && !core::string::icontains(voxelFile.license, _currentFilterLicense)) {
		return true;
	}
	if (_currentFilterFormatEntry <= 0) {
		return false;
	}
	const core::String &filter = _filterEntries[_currentFilterFormatEntry].wildCard();
	if (core::string::fileMatchesMultiple(voxelFile.name.c_str(), filter.c_str())) {
		return false;
	}
	return true;
}

bool CollectionPanel::isFilterActive() const {
	return !_currentFilterName.empty() || !_currentFilterLicense.empty() || _currentFilterFormatEntry > 0;
}

void CollectionPanel::updateFilters() {
	{
		const ImVec2 itemWidth = ImGui::CalcTextSize("#########");
		ImGui::PushItemWidth(itemWidth.x);
		ImGui::InputText(_("Name"), &_currentFilterName);
		ImGui::PopItemWidth();
		ImGui::SameLine();
	}
	{
		const ImVec2 itemWidth = ImGui::CalcTextSize("#########");
		ImGui::PushItemWidth(itemWidth.x);
		ImGui::InputText(_("License"), &_currentFilterLicense);
		ImGui::PopItemWidth();
		ImGui::SameLine();
	}
	{
		if (_filterFormatTextWidth < 0.0f) {
			for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
				_filterEntries.push_back(*desc);
				const core::String &str = io::convertToFilePattern(*desc);
				const ImVec2 filterTextSize = ImGui::CalcTextSize(str.c_str());
				_filterFormatTextWidth = core_max(_filterFormatTextWidth, filterTextSize.x);
			}
			_filterEntries.sort(core::Greater<io::FormatDescription>());
			io::createGroupPatterns(voxelformat::voxelLoad(), _filterEntries);
			// must be the first entry - see applyFilter()
			_filterEntries.insert(_filterEntries.begin(), io::ALL_SUPPORTED());
		}

		const char *formatFilterLabel = _("Format");
		ImGui::PushItemWidth(_filterFormatTextWidth);
		int currentlySelected = _currentFilterFormatEntry == -1 ? 0 : _currentFilterFormatEntry;
		const core::String &selectedEntry = io::convertToFilePattern(_filterEntries[currentlySelected]);

		if (ImGui::BeginCombo(formatFilterLabel, selectedEntry.c_str(), ImGuiComboFlags_HeightLargest)) {
			for (int i = 0; i < (int)_filterEntries.size(); ++i) {
				const bool selected = i == currentlySelected;
				const io::FormatDescription &format = _filterEntries[i];
				const core::String &text = io::convertToFilePattern(format);
				if (ImGui::Selectable(text.c_str(), selected)) {
					_currentFilterFormatEntry = i;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
	}
}

int CollectionPanel::update(const voxelcollection::VoxelFileMap &voxelFilesMap,
							const std::function<void(VoxelFile &voxelFile)> &contextMenu) {
	int cnt = 0;
	if (ImGui::BeginChild("##collectionpanel")) {
		updateFilters();

		if (ImGui::BeginTable(_("Voxel Files"), 3,
							  ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders |
								  ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn(_("Thumbnail"));
			ImGui::TableSetupColumn(_("Name"));
			ImGui::TableSetupColumn(_("License"));
			ImGui::TableHeadersRow();
			for (const auto &entry : voxelFilesMap) {
				ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAllColumns |
											   ImGuiTreeNodeFlags_SpanAvailWidth;
				if (isFilterActive()) {
					treeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
				}
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				const int n = (int)entry->second.files.size();
				const core::String &label = core::string::format("%s (%i)", entry->first.c_str(), n);
				ImGui::BeginDisabled(!entry->second.sorted);
				if (ImGui::TreeNodeEx(label.c_str(), treeFlags)) {
					const voxelcollection::VoxelFiles &voxelFiles = entry->second.files;
					cnt += buildVoxelTree(voxelFiles, contextMenu);
					ImGui::TreePop();
				}
				ImGui::EndDisabled();
			}
			ImGui::EndTable();
		}
	}
	ImGui::EndChild();
	return cnt;
}

int CollectionPanel::buildVoxelTree(const voxelcollection::VoxelFiles &voxelFiles,
									const std::function<void(VoxelFile &voxelFile)> &contextMenu) {
	core::DynamicArray<voxelcollection::VoxelFile *> f;
	f.reserve(voxelFiles.size());

	for (voxelcollection::VoxelFile &voxelFile : voxelFiles) {
		if (filtered(voxelFile)) {
			continue;
		}
		f.push_back(&voxelFile);
	}

	ImGuiListClipper clipper;
	clipper.Begin((int)f.size());

	_newSelected = false;
	while (clipper.Step()) {
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
			voxelcollection::VoxelFile *voxelFile = f[row];

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			const bool selected = _selected == *voxelFile;

			ImGui::PushID(voxelFile->targetFile().c_str());
			if (ImGui::Selectable("##invis", selected, ImGuiSelectableFlags_SpanAllColumns)) {
				_selected = *voxelFile;
				_newSelected = true;
			}

			if (contextMenu) {
				if (ImGui::BeginPopupContextItem()) {
					contextMenu(*voxelFile);
					ImGui::EndPopup();
				}
			}

			video::Id handle;
			if (const video::TexturePtr &texture = thumbnailLookup(*voxelFile)) {
				handle = texture->handle();
			} else {
				handle = video::InvalidId;
			}
			ImGui::Image(handle, ImVec2(64, 64));
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
			ImGui::PopID();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(voxelFile->name.c_str());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(voxelFile->license.c_str());
		}
	}

	return (int)f.size();
}

video::TexturePtr CollectionPanel::thumbnailLookup(const voxelcollection::VoxelFile &voxelFile) {
	static video::TexturePtr empty;
	if (_texturePool->has(voxelFile.name)) {
		return _texturePool->get(voxelFile.name);
	}
	return empty;
}

void CollectionPanel::shutdown() {
	_filterEntries.clear();
}

bool CollectionPanel::init() {
	return true;
}

voxelcollection::VoxelFile &CollectionPanel::selected() {
	return _selected;
}

} // namespace voxelcollection
