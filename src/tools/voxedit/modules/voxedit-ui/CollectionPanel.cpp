/**
 * @file
 */

#include "CollectionPanel.h"
#include "IconsLucide.h"
#include "command/CommandHandler.h"
#include "core/StringUtil.h"
#include "ui/IMGUIEx.h"
#include "ui/IMGUIApp.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxelcollection/Downloader.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelui/DragAndDropPayload.h"

namespace voxedit {

CollectionPanel::CollectionPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr,
								 const voxelcollection::CollectionManagerPtr &collectionMgr,
								 const video::TexturePoolPtr &texturePool)
	: Super(app, "collection"), _sceneMgr(sceneMgr), _collectionMgr(collectionMgr), _texturePool(texturePool) {
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
	const float itemWidth = ImGui::Size(9.0f);
	{
		ImGui::PushItemWidth(itemWidth);
		ImGui::InputText(_("Name"), &_currentFilterName);
		ImGui::PopItemWidth();
		ImGui::SameLine();
	}
	{
		ImGui::PushItemWidth(itemWidth);
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
			_filterFormatTextWidth = core_min(itemWidth * 2.0f, _filterFormatTextWidth);
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

int CollectionPanel::update() {
	core_trace_scoped(CollectionPanel);
	int cnt = 0;
	const voxelcollection::VoxelFileMap &voxelFilesMap = _collectionMgr->voxelFilesMap();
	updateFilters();

	if (ImGui::IconButton(ICON_LC_FOLDER, _("Local directory"))) {
		_app->directoryDialog([&] (const core::String &folderName, const io::FormatDescription *desc) {
			_collectionMgr->setLocalDir(folderName);
		}, {});
	}
	ImGui::TooltipTextUnformatted(_collectionMgr->localDir().c_str());

	const int columns = _thumbnails ? 3 : 2;
	if (ImGui::BeginTable("##voxelfiles", columns,
						  ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders |
							  ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
		ImGui::TableSetupScrollFreeze(0, 1);
		if (_thumbnails) {
			ImGui::TableSetupColumn(_("Thumbnail"));
		}
		ImGui::TableSetupColumn(_("Name"));
		ImGui::TableSetupColumn(_("License"));
		ImGui::TableHeadersRow();
		for (const auto &source : _collectionMgr->sources()) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAllColumns |
										   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed;
			auto iter = voxelFilesMap.find(source.name);
			if (iter != voxelFilesMap.end()) {
				const voxelcollection::VoxelCollection &collection = iter->second;
				if (isFilterActive() && _collectionMgr->resolved(source)) {
					treeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
				} else if (!collection.sorted) {
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				}
				const int n = (int)collection.files.size();
				const core::String &label = core::String::format("%s (%i)##%s", source.name.c_str(), n, source.name.c_str());
				if (ImGui::TreeNodeEx(label.c_str(), treeFlags)) {
					if (!collection.sorted) {
						ImGui::Spinner("##collectionspinner", ImGui::Size(1.0f));
						ImGui::SameLine();
						ImGui::TextUnformatted(_("Loading..."));
					} else {
						const voxelcollection::VoxelFiles &voxelFiles = collection.files;
						cnt += buildVoxelTree(voxelFiles);
					}
					ImGui::TreePop();
				}
				if (source.isLocal()) {
					ImGui::TooltipTextUnformatted(_collectionMgr->localDir().c_str());
				}
			} else {
				if (!_collectionMgr->resolved(source)) {
					treeFlags |= ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_Bullet;
				}
				if (ImGui::TreeNodeEx(source.name.c_str(), treeFlags)) {
					// if resolved already but no files are available, we are still loading...
					if (_collectionMgr->resolved(source)) {
						ImGui::Spinner("##sourcespinner", ImGui::Size(1.0f));
						ImGui::SameLine();
						ImGui::TextUnformatted(_("Loading..."));
					} else {
						_collectionMgr->resolve(source);
					}
					ImGui::TreePop();
				} else {
					ImGui::TooltipTextUnformatted(_("Double click to load"));
				}
			}
		}
		ImGui::EndTable();
	}
	return cnt;
}

void CollectionPanel::contextMenu(voxelcollection::VoxelFile *voxelFile) {
	if (ImGui::BeginPopupContextItem()) {
		if (!voxelFile->downloaded) {
			_collectionMgr->download(*voxelFile);
		}

		if (ImGui::MenuItem(_("Use stamp"))) {
			Modifier &modifier = _sceneMgr->modifier();
			StampBrush &brush = modifier.stampBrush();
			if (brush.load(voxelFile->targetFile())) {
				modifier.setBrushType(BrushType::Stamp);
			} else {
				Log::error("Failed to load stamp brush");
			}
		}
		ImGui::TooltipTextUnformatted(
			_("This is only possible if the model doesn't exceed the max allowed stamp size"));

		if (ImGui::MenuItem(_("Add to scene"))) {
			import(voxelFile);
		}

		if (_thumbnails) {
			if (ImGui::MenuItem(_("Hide thumbnails"))) {
				_thumbnails = false;
			}
		} else {
			if (ImGui::MenuItem(_("Show thumbnails"))) {
				_thumbnails = true;
			}
		}

		if (!io::isA(voxelFile->name, voxelformat::voxelLoad())) {
			if (ImGui::MenuItem(_("Open target file"))) {
				core::String absPath = _collectionMgr->absolutePath(*voxelFile);
				command::executeCommands("url \"file://" + absPath + "\"");
			}
			if (ImGui::MenuItem(_("Open target dir"))) {
				core::String absPath = _collectionMgr->absolutePath(*voxelFile);
				command::executeCommands("url \"file://" + core::string::extractDir(absPath) + "\"");
			}
		} else if (!thumbnailLookup(*voxelFile)) {
			if (ImGui::MenuItem(_("Create thumbnail"))) {
				_collectionMgr->createThumbnail(*voxelFile);
			}
		}

		ImGui::EndPopup();
	}
}

bool CollectionPanel::import(voxelcollection::VoxelFile *voxelFile) {
	if (!voxelFile->downloaded) {
		_collectionMgr->download(*voxelFile);
	}
	if (!voxelFile->downloaded) {
		return false;
	}
	Log::debug("Try to import %s", voxelFile->targetFile().c_str());
	return _sceneMgr->import(voxelFile->targetFile());
}

void CollectionPanel::handleDoubleClick(voxelcollection::VoxelFile *voxelFile) {
	if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		import(voxelFile);
		_selected = *voxelFile;
	}
}

void CollectionPanel::thumbnailTooltip(voxelcollection::VoxelFile *voxelFile) {
	if (const video::TexturePtr &texture = thumbnailLookup(*voxelFile)) {
		if (ImGui::BeginItemTooltip()) {
			const video::Id handle = texture->handle();
			ImGui::Image(handle, ImGui::Size(40.0f));
			ImGui::TextUnformatted(voxelFile->fullPath.c_str());
			ImGui::EndTooltip();
		}
	}
}

void CollectionPanel::handleDragAndDrop(int row, voxelcollection::VoxelFile *voxelFile) {
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
		video::Id handle;
		if (const video::TexturePtr &texture = thumbnailLookup(*voxelFile)) {
			handle = texture->handle();
		} else {
			handle = video::InvalidId;
		}
		char mdlId[64];
		core::String::formatBuf(mdlId, sizeof(mdlId), "%i", row);
		ImGui::ImageButton(mdlId, handle, ImVec2(50, 50));
		_dragAndDropModel = voxelFile->targetFile();
		ImGui::SetDragDropPayload(voxelui::dragdrop::ModelPayload, (const void *)&_dragAndDropModel,
								  sizeof(_dragAndDropModel), ImGuiCond_Always);
		ImGui::EndDragDropSource();
	}
}

int CollectionPanel::buildVoxelTree(const voxelcollection::VoxelFiles &voxelFiles) {
	core::Buffer<voxelcollection::VoxelFile *> f;
	f.reserve(voxelFiles.size());

	for (voxelcollection::VoxelFile &voxelFile : voxelFiles) {
		if (filtered(voxelFile)) {
			continue;
		}
		f.push_back(&voxelFile);
	}
	if (f.empty()) {
		return 0;
	}

	ImGuiListClipper clipper;
	clipper.Begin((int)f.size());

	const bool thumbnails = _thumbnails;
	while (clipper.Step()) {
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
			voxelcollection::VoxelFile *voxelFile = f[row];

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			const bool selected = _selected == *voxelFile;
			const ImGuiStyle &style = ImGui::GetStyle();

			ImVec2 size(0, ImGui::GetFontSize());
			ImGui::PushID(row);
			if (thumbnails) {
				video::Id handle;
				if (const video::TexturePtr &texture = thumbnailLookup(*voxelFile)) {
					handle = texture->handle();
				} else {
					handle = video::InvalidId;
				}
				const float w = core_max(64, ImGui::Size(8));
				size = ImVec2(w, w);
				if (ImGui::ImageButton("##thumbnail", handle, size)) {
					if (!voxelFile->downloaded) {
						_collectionMgr->download(*voxelFile);
					}
					if (handle == video::InvalidId) {
						_collectionMgr->createThumbnail(*voxelFile);
					}
				}
				handleDragAndDrop(row, voxelFile);

				if (handle == video::InvalidId) {
					if (ImGui::BeginItemTooltip()) {
						ImGui::TextUnformatted(_("Double click to create thumbnail"));
						ImGui::TextUnformatted(voxelFile->fullPath.c_str());
						ImGui::EndTooltip();
					}
				} else {
					thumbnailTooltip(voxelFile);
				}
				ImGui::TableNextColumn();
			}
			if (ImGui::Selectable(voxelFile->name.c_str(), selected,
								  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick,
								  {0, size.y + style.FramePadding.y * 2.0f})) {
				handleDoubleClick(voxelFile);
			}
			handleDragAndDrop(row, voxelFile);
			if (!thumbnails) {
				thumbnailTooltip(voxelFile);
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}

			contextMenu(voxelFile);
			ImGui::PopID();

			ImGui::TableNextColumn();
			ImGui::TextUnformatted(voxelFile->license.c_str());
		}
	}

	if (f.empty() && isFilterActive()) {
		ImGui::TextUnformatted(_("No match for filter found"));
	}
	return (int)f.size();
}

video::TexturePtr CollectionPanel::thumbnailLookup(const voxelcollection::VoxelFile &voxelFile) {
	static video::TexturePtr empty;
	const core::String &id = voxelFile.id();
	if (_texturePool->has(id)) {
		return _texturePool->get(id);
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

} // namespace voxedit
