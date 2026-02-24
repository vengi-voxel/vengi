/**
 * @file
 */

#include "FileDialog.h"
#include "IconsLucide.h"
#include "ScopedStyle.h"
#include "app/App.h"
#include "app/Async.h"
#include "core/Algorithm.h"
#include "core/Alphanumeric.h"
#include "core/ArrayLength.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "dearimgui/imgui_internal.h"
#include "io/Filesystem.h"
#include "io/FilesystemEntry.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIEx.h"
#ifdef __EMSCRIPTEN__
#include "io/MemoryReadStream.h"
#endif
#include "ui/DragAndDropPayload.h"
#include "ui/IMGUIApp.h"
#include "video/OpenFileMode.h"

namespace ui {

enum class FileDialogColumnId {
	File,
	Size,
	Type,
	Date,

	Max
};

static const struct FileDialogSorter {
	bool (*asc)(const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs);
	bool (*desc)(const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs);
} fileDialogSorter[(int)FileDialogColumnId::Max] = {
	{[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) {
		if (lhs->name == "..") {
			return false;
		} else if (rhs->name == "..") {
			return true;
		}
		return core::Alphanumeric(lhs->name.c_str()) < core::Alphanumeric(rhs->name.c_str());
	 },
	 [](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) {
		if (lhs->name == "..") {
			return false;
		} else if (rhs->name == "..") {
			return true;
		}
		return core::Alphanumeric(lhs->name.c_str()) > core::Alphanumeric(rhs->name.c_str());
	 }},
	{[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) {
		if (lhs->name == "..") {
			return false;
		} else if (rhs->name == "..") {
			return true;
		}
		return lhs->size < rhs->size;
	 },
	 [](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) {
		if (lhs->name == "..") {
			return false;
		} else if (rhs->name == "..") {
			return true;
		}
		return lhs->size > rhs->size;
	 }},
	{[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) {
		if (lhs->name == "..") {
			return false;
		} else if (rhs->name == "..") {
			return true;
		}
		const core::String &aext = core::string::extractExtension(lhs->name);
		const core::String &bext = core::string::extractExtension(rhs->name);
		return core::Alphanumeric(aext.c_str()) < core::Alphanumeric(bext.c_str());
	 },
	 [](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) {
		if (lhs->name == "..") {
			return false;
		} else if (rhs->name == "..") {
			return true;
		}
		const core::String &aext = core::string::extractExtension(lhs->name);
		const core::String &bext = core::string::extractExtension(rhs->name);
		return core::Alphanumeric(aext.c_str()) > core::Alphanumeric(bext.c_str());
	 }},
	{[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) {
		if (lhs->name == "..") {
			return false;
		} else if (rhs->name == "..") {
			return true;
		}
		return lhs->mtime < rhs->mtime;
	 },
	 [](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) {
		if (lhs->name == "..") {
			return false;
		} else if (rhs->name == "..") {
			return true;
		}
		return lhs->mtime > rhs->mtime;
	 }}};

static core::String assemblePath(const core::String &dir, const io::FilesystemEntry &ent) {
	if (ent.isDirectory() && ent.name == "..") {
		return ent.fullPath;
	}
	return core::string::path(dir, ent.name);
}

void FileDialog::applyFilter(video::OpenFileMode type) {
	_filteredEntities.clear();
	_filteredEntities.reserve(_entities.size() + 1);
	const bool isRootPath = core::string::isRootPath(_currentPath);
	if (!isRootPath) {
		_parentDir.name = "..";
		_parentDir.type = io::FilesystemEntry::Type::dir;
		_parentDir.fullPath = _app->filesystem()->sysAbsolutePath(core::string::path(_currentPath, ".."));
		_filteredEntities.push_back(&_parentDir);
	}
	for (size_t i = 0; i < _entities.size(); ++i) {
		if (hide(_entities[i].fullPath)) {
			continue;
		}
		if (_entities[i].isDirectory()) {
			_filteredEntities.push_back(&_entities[i]);
			continue;
		} else if (type == video::OpenFileMode::Directory) {
			continue;
		}
		if (_currentFilterFormat) {
			// this is "all-supported files"
			const bool showAll = !_filterAll.empty() && _currentFilterEntry == 0;
			const core::String &filter = showAll ? _filterAll : _currentFilterFormat->wildCard();
			if (!core::string::fileMatchesMultiple(_entities[i].name.c_str(), filter.c_str())) {
				continue;
			}
		}
		_filteredEntities.push_back(&_entities[i]);
	}

	_needsSorting = true;

	if (_currentFilterFormat && type == video::OpenFileMode::Save) {
		const io::FormatDescription &desc = *_currentFilterFormat;
		const core::String &extension = core::string::extractExtension(_selectedEntry.name);
		if (!desc.exts.empty() && !desc.matchesExtension(extension)) {
			_selectedEntry.setExtension(desc.exts[0]);
		}
	}
}

void FileDialog::selectFilter(video::OpenFileMode type, int index) {
	core_assert(index >= -1 && index <= (int)_filterEntries.size());
	_currentFilterEntry = index;
	if (type == video::OpenFileMode::Open) {
		_lastFilterOpen->setVal(_currentFilterEntry);
	} else if (type == video::OpenFileMode::Save) {
		_lastFilterSave->setVal(_currentFilterEntry);
	}
	if (_currentFilterEntry != -1) {
		_currentFilterFormat = &_filterEntries[_currentFilterEntry];
	} else {
		_currentFilterFormat = nullptr;
	}
	applyFilter(type);
}

#ifdef __EMSCRIPTEN__
void FileDialog::uploadHandler(std::string const& filename, std::string const& mimetype, std::string_view buffer, void* userdata) {
	io::MemoryReadStream stream(buffer.data(), buffer.size());
	FileDialog *fileDialog = (FileDialog*)userdata;
	io::filesystem()->homeWrite(filename.c_str(), stream);
	fileDialog->readDir(video::OpenFileMode::Open);
}
#endif

bool FileDialog::openDir(video::OpenFileMode type, const io::FormatDescription* formats, const core::String& filename) {
	_filterEntries.clear();
	if (formats == nullptr) {
		_filterTextWidth = 0.0f;
		_filterAll = "";
		_currentFilterEntry = -1;
		_currentFilterFormat = nullptr;
	} else {
		_filterTextWidth = 0.0f;
		const io::FormatDescription* f = formats;
		while (f->valid()) {
			const core::String& str = io::convertToFilePattern(*f);
			const ImVec2 filterTextSize = ImGui::CalcTextSize(str.c_str());
			_filterTextWidth = core_max(_filterTextWidth, filterTextSize.x);
			_filterEntries.push_back(*f);
			++f;
		}
		app::sort_parallel(_filterEntries.begin(), _filterEntries.end(), core::Less<io::FormatDescription>());
		if (type == video::OpenFileMode::Open) {
			io::createGroupPatterns(formats, _filterEntries);
		}
		_filterAll = io::convertToAllFilePattern(formats);
		if (!_filterAll.empty()) {
			// must be the first entry - see applyFilter()
			_filterEntries.insert(_filterEntries.begin(), io::ALL_SUPPORTED());
		}

		int lastFilter = 0;
		if (type == video::OpenFileMode::Open) {
			lastFilter = _lastFilterOpen->intVal();
		} else if (type == video::OpenFileMode::Save) {
			lastFilter = _lastFilterSave->intVal();
		}
		if (lastFilter < 0 || lastFilter >= (int)_filterEntries.size()) {
			lastFilter = 0;
		}
		selectFilter(type, lastFilter);
	}

	const core::String &filePath = core::string::extractDir(filename);
	if (filePath.empty() || !_app->filesystem()->exists(filePath)) {
		const core::String &lastDir = _lastDirVar->strVal();
		if (_app->filesystem()->exists(lastDir)) {
			_currentPath = lastDir;
		} else {
			_currentPath = _app->filesystem()->homePath();
		}
	} else {
		_currentPath = filePath;
	}
	_selectedEntry = io::FilesystemEntry{core::string::extractFilenameWithExtension(filename), filename, io::FilesystemEntry::Type::file, 0, 0};
	_entryIndex = -1;

	if (!_app->filesystem()->exists(_currentPath)) {
		_currentPath = _app->filesystem()->homePath();
		_lastDirVar->setVal(_currentPath);
	}

#ifdef __EMSCRIPTEN__
	if (type == video::OpenFileMode::Open) {
		emscripten_browser_file::upload("", uploadHandler, this);
	}
#endif

	return readDir(type);
}

bool FileDialog::readDir(video::OpenFileMode type) {
	_type = type;
	_entities.clear();
	if (!_app->filesystem()->list(_currentPath, _entities)) {
		Log::warn("Failed to list dir %s", _currentPath.c_str());
		return false;
	}

	applyFilter(type);
	return true;
}

bool FileDialog::quickAccessEntry(int index, video::OpenFileMode type, const core::String& path, float width, const char *title, const char *icon) {
	if (path.empty()) {
		return false;
	}
	core::String quickAccessTitle;
	if (title == nullptr) {
		quickAccessTitle = path;
		if (quickAccessTitle.size() > 1u) {
			if (quickAccessTitle.last() == '/') {
				quickAccessTitle.erase(quickAccessTitle.size() - 1);
			}
			const size_t pos = quickAccessTitle.rfind("/");
			if (pos != core::String::npos) {
				quickAccessTitle = quickAccessTitle.substr(pos + 1);
			}
		}
	} else {
		quickAccessTitle = title;
	}
	quickAccessTitle += core::String::format("###%i", index);
	if (icon != nullptr) {
		const float x = ImGui::GetCursorPosX();
		ImGui::TextUnformatted(icon);
		ImGui::SameLine();
		ImGui::SetCursorPosX(x + 2.0f * ImGui::GetStyle().ItemInnerSpacing.x + 1.5f * ImGui::GetFontSize());
	}
	const ImVec2 size(width, 0);
	if (ImGui::Selectable(quickAccessTitle.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, size)) {
		setCurrentPath(type, path);
	}
	ImGui::TooltipTextUnformatted(path.c_str());
	return true;
}

void FileDialog::removeBookmark(const core::String &bookmark) {
	core::VarPtr bookmarks = core::getVar(cfg::UIBookmarks);

	core::DynamicArray<core::String> bm;
	core::string::splitString(bookmarks->strVal(), bm, ";");
	core::String newBookmarks;
	newBookmarks.reserve(bookmarks->strVal().size());
	for (const core::String &path : bm) {
		if (path == bookmark) {
			continue;
		}
		if (!newBookmarks.empty()) {
			newBookmarks.append(";");
		}
		newBookmarks.append(path);
	}
	bookmarks->setVal(newBookmarks);
}

bool FileDialog::quickAccessPanel(video::OpenFileMode type, const core::String &bookmarks, int height) {
	ScopedStyle style;
	style.setItemSpacing(ImVec2(10.0f, 10.0f));
	const float width = ImGui::Size(30.0f);
	ImGui::BeginChild("bookmarks_child", ImVec2(width, height), ImGuiChildFlags_Borders);
	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float contentRegionWidth = available.x + ImGui::GetCursorPosX();

	const char *folderNames[] = {_("Download"), _("Desktop"), _("Documents"), _("Pictures"), _("Public"), _("Fonts"), _("Recent"), _("Cloud")};
	static const char *folderIcons[] = {ICON_LC_DOWNLOAD, ICON_LC_MONITOR_DOT, ICON_LC_FILE, ICON_LC_IMAGE, ICON_LC_FOLDER, ICON_LC_FOLDER, ICON_LC_FOLDER, ICON_LC_CLOUD};
	static_assert(lengthof(folderNames) == io::FilesystemDirectories::FS_Dir_Max, "Array size doesn't match enum value");
	static_assert(lengthof(folderIcons) == io::FilesystemDirectories::FS_Dir_Max, "Array size doesn't match enum value");

	int index = 0;
	if (ImGui::TreeNode(_("Quick Access"))) {
		for (int n = 0; n < io::FilesystemDirectories::FS_Dir_Max; ++n) {
			const core::String& dir = _app->filesystem()->sysSpecialDir((io::FilesystemDirectories)n);
			if (dir.empty()) {
				continue;
			}
			quickAccessEntry(index++, type, dir, contentRegionWidth, folderNames[n], folderIcons[n]);
		}
		const io::Paths& paths = _app->filesystem()->registeredPaths();
		for (const core::String& path : paths) {
			const core::String& absPath = _app->filesystem()->sysAbsolutePath(path);
			if (absPath.empty()) {
				continue;
			}
			quickAccessEntry(index++, type, absPath, contentRegionWidth, nullptr, ICON_LC_FOLDER);
		}
		ImGui::TreePop();
	}

	if (!_app->filesystem()->sysOtherPaths().empty()) {
		if (ImGui::TreeNode(_("This PC"))) {
			for (const io::ThisPCEntry &entry : _app->filesystem()->sysOtherPaths()) {
				quickAccessEntry(index++, type, entry.path, contentRegionWidth, entry.name.c_str(), ICON_LC_FOLDER);
			}
			ImGui::TreePop();
		}
	}

	bool openFileFromRecent = false;
	if (ImGui::TreeNode(_("Recent"))) {
		for (const core::String &file : _app->lastOpenedFiles()) {
			if (file.empty()) {
				continue;
			}
			const core::String basename = core::string::extractFilenameWithExtension(file);
			const float x = ImGui::GetCursorPosX();
			ImGui::TextUnformatted(ICON_LC_FILE);
			ImGui::SameLine();
			ImGui::SetCursorPosX(x + 2.0f * ImGui::GetStyle().ItemInnerSpacing.x + 1.5f * ImGui::GetFontSize());
			const ImVec2 size(width, 0);
			if (ImGui::Selectable(basename.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, size)) {
				openFileFromRecent = true;
				_selectedEntry = io::createFilesystemEntry(file);
			}
			ImGui::TooltipTextUnformatted(file.c_str());
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode(_("Bookmarks"))) {
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(dragdrop::FileDialogDirectoryPayload)) {
				const core::String &directory = *(core::String *)payload->Data;
				addBookmark(directory);
			}
			ImGui::EndDragDropTarget();
		}
		core::DynamicArray<core::String> bm;
		core::string::splitString(bookmarks, bm, ";");
		for (const core::String& path : bm) {
			const core::String& absPath = _app->filesystem()->sysAbsolutePath(path);
			if (absPath.empty()) {
				removeBookmark(path);
				continue;
			}
			if (quickAccessEntry(index++, type, absPath, contentRegionWidth, nullptr, ICON_LC_FOLDER)) {
				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::IconButton(ICON_LC_TRASH, _("Remove bookmark"))) {
						removeBookmark(path);
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
			}
		}
		ImGui::TreePop();
	}

	ImGui::EndChild();

	return openFileFromRecent;
}

void FileDialog::setCurrentPath(video::OpenFileMode type, const core::String& path) {
	resetState();
	_currentPath = path;
	core::getVar(cfg::UILastDirectory)->setVal(_currentPath);
	readDir(type);
}

bool FileDialog::hide(const core::String &file) const {
	if (_showHidden->boolVal()) {
		return false;
	}
	return io::Filesystem::sysIsHidden(file);
}

static const char *iconForType(io::FilesystemEntry::Type type) {
	switch (type) {
	case io::FilesystemEntry::Type::dir:
		return ICON_LC_FOLDER;
	case io::FilesystemEntry::Type::file:
		return ICON_LC_FILE_PLUS;
	case io::FilesystemEntry::Type::link:
		return ICON_LC_LINK;
	default:
		break;
	}
	return "";
}

bool FileDialog::entitiesPanel(video::OpenFileMode type, int height) {
	ImVec2 childSize(ImGui::GetContentRegionAvail().x, height);
	ImGui::BeginChild("files", childSize, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);

	bool doubleClickedFile = false;
	bool doubleClickedDir = false;
	io::FilesystemEntry doubleClickedDirEntry;
	uint32_t tableFlags =
		ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable |
		ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
	if (_filteredEntities.size() < 2000) {
		tableFlags |= ImGuiTableFlags_Sortable;
	}
	if (ImGui::BeginTable("##files", 4, tableFlags)) {
		ImGui::TableSetupColumn(_("File"), ImGuiTableColumnFlags_WidthStretch, 0.7f, (int)FileDialogColumnId::File);
		ImGui::TableSetupColumn(_("Size"), ImGuiTableColumnFlags_WidthStretch, 0.09f, (int)FileDialogColumnId::Size);
		ImGui::TableSetupColumn(_("Type"), ImGuiTableColumnFlags_WidthStretch, 0.07f, (int)FileDialogColumnId::Type);
		ImGui::TableSetupColumn(_("Date"), ImGuiTableColumnFlags_WidthStretch, 0.14f, (int)FileDialogColumnId::Date);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		// Sort files
		if (ImGuiTableSortSpecs *specs = ImGui::TableGetSortSpecs()) {
			if ((_needsSorting || specs->SpecsDirty) && _filteredEntities.size() > 1U) {
				for (int n = 0; n < specs->SpecsCount; n++) {
					const ImGuiTableColumnSortSpecs &spec = specs->Specs[n];
					if (spec.SortDirection == ImGuiSortDirection_Ascending) {
						_filteredEntities.sort(fileDialogSorter[spec.ColumnUserID].asc);
					} else {
						_filteredEntities.sort(fileDialogSorter[spec.ColumnUserID].desc);
					}
					break;
				}
				_needsSorting = specs->SpecsDirty = false;
			}
		}

		// add filtered and sorted directory entries
		ImGuiListClipper clipper;
		clipper.Begin((int)_filteredEntities.size());
		if (_scrollToSelection) {
			clipper.IncludeItemByIndex(_entryIndex);
		}
		while (clipper.Step()) {
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
				const io::FilesystemEntry entry = *_filteredEntities[i];
				ImGui::TableNextColumn();

				const bool selected = i == (int)_entryIndex;
				if (selected && _scrollToSelection) {
					_scrollToSelection = false;
					ImGui::SetScrollHereY();
				}
				if (selected && entry.name != _parentDir.name) {
					_selectedEntry = entry;
				}
				const char *icon = iconForType(entry.type);
				const float x = ImGui::GetCursorPosX();
				ImGui::TextUnformatted(icon);
				ImGui::SameLine();
				ImGui::SetCursorPosX(x + 1.5f * ImGui::GetFontSize());
				if (ImGui::Selectable(entry.name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						if (entry.isDirectory()) {
							doubleClickedDir = true;
							doubleClickedDirEntry = entry;
						} else {
							doubleClickedFile = true;
							resetState();
							_entryIndex = i;
							_selectedEntry = entry;
						}
					} else if (entry.name != _parentDir.name) {
						resetState();
						_entryIndex = i;
						_selectedEntry = entry;
					}
				}
				if (entry.isDirectory()) {
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						_dragAndDropName = core::string::path(_currentPath, entry.name);
						ImGui::TextUnformatted(_dragAndDropName.c_str());
						ImGui::SetDragDropPayload(dragdrop::FileDialogDirectoryPayload, &_dragAndDropName, sizeof(core::String),
													ImGuiCond_Always);
						ImGui::EndDragDropSource();
					}
				}
				ImGui::TableNextColumn();
				const core::String &humanSize = core::string::humanSize(entry.size);
				ImGui::TextUnformatted(humanSize.c_str());
				ImGui::TableNextColumn();
				if (entry.isLink()) {
					ImGui::TextUnformatted(_("link"));
				} else if (entry.isDirectory()) {
					ImGui::TextUnformatted(_("directory"));
				} else {
					const core::String &fileExt = core::string::extractExtension(entry.name);
					if (fileExt.empty()) {
						ImGui::TextUnformatted("-");
					} else {
						ImGui::TextUnformatted(fileExt.c_str());
					}
				}
				ImGui::TableNextColumn();
				const core::String &lastModified = core::TimeProvider::toString(entry.mtime);
				ImGui::TextUnformatted(lastModified.c_str());
			}
		}
		ImGui::EndTable();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::GetCurrentContext()->PlatformImeData.WantTextInput = true;
	}
	ImGui::EndChild();

	if (doubleClickedDir) {
		setCurrentPath(type, assemblePath(_currentPath, doubleClickedDirEntry));
	}

	return doubleClickedFile;
}

void FileDialog::addBookmark(const core::String &bookmark) {
	Log::debug("Add new bookmark: %s", bookmark.c_str());
	removeBookmark(bookmark);
	core::String bm = _bookmarks->strVal();
	if (bm.empty()) {
		bm.append(bookmark);
	} else {
		bm.append(";" + bookmark);
	}
	_bookmarks->setVal(bm);
}

void FileDialog::currentPathPanel(video::OpenFileMode type) {
	if (ImGui::Button(ICON_LC_BOOKMARK "###addbookmark")) {
		addBookmark(_currentPath);
	}
	ImGui::TooltipTextUnformatted(_("Add a bookmark for the current active folder"));

	ImGui::SameLine();

	core::Path p(_currentPath);
	const core::DynamicArray<core::String> &components = p.components();
	ImGui::TextUnformatted(">");
	core::Path path("/");
#ifdef _WIN32
	path = core::Path("");
#endif
	int i = 0;
	for (const core::String &c : components) {
		path = path.append(c);
		ImGui::PushID(i);
		ImGui::SameLine();
		if (ImGui::Button(c.c_str())) {
			setCurrentPath(type, path.str());
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			_dragAndDropName = path.str();
			ImGui::TextUnformatted(_dragAndDropName.c_str());
			ImGui::SetDragDropPayload(dragdrop::FileDialogDirectoryPayload, &_dragAndDropName, sizeof(core::String),
									  ImGuiCond_Always);
			ImGui::EndDragDropSource();
		}
		ImGui::TooltipTextUnformatted(path.c_str());

		ImGui::PopID();
		++i;
	}
}

void FileDialog::construct() {
	const core::VarDef uIBookmarks(cfg::UIBookmarks, "");
	_bookmarks = core::Var::registerVar(uIBookmarks);
	const core::VarDef uIFileDialogShowHidden(cfg::UIFileDialogShowHidden, false, -1, _("Show hidden file system entities"));
	_showHidden = core::Var::registerVar(uIFileDialogShowHidden);
	const core::VarDef uILastDirectory(cfg::UILastDirectory, _app->filesystem()->homePath().c_str());
	_lastDirVar = core::Var::registerVar(uILastDirectory);
	const core::VarDef uILastFilterSave(cfg::UILastFilterSave, 0, -1, _("The last selected file type filter in the file dialog"));
	_lastFilterSave = core::Var::registerVar(uILastFilterSave);
	const core::VarDef uILastFilterOpen(cfg::UILastFilterOpen, 0, -1, _("The last selected file type filter in the file dialog"));
	_lastFilterOpen = core::Var::registerVar(uILastFilterOpen);
}

void FileDialog::resetState() {
	_entryIndex = 0;
	const bool isRootPath = core::string::isRootPath(_currentPath);
	if (!isRootPath) {
		++_entryIndex;
	}

	_selectedEntry = {};
	_scrollToText = {};
	_error = {};
}

void FileDialog::popupNotWriteable() {
	const core::String title = makeTitle(_("Not writeable"), FILE_NOT_WRITEABLE_POPUP);
	if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::IconDialog(ICON_LC_TRIANGLE_ALERT, _("The selected file or directory is not writeable"));
		if (ImGui::OkButton()) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void FileDialog::popupNewFolder() {
	const ImVec2 &windowPos = ImGui::GetWindowPos();
	const ImVec2 &windowSize = ImGui::GetWindowSize();
	const ImVec2 center(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	const core::String title = makeTitle(_("Create folder"), NEW_FOLDER_POPUP);

	if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted(_("Enter a name for the new folder"));
		ImGui::InputText("##newfoldername", &_newFolderName.name);
		if (ImGui::Button(_("Create"))) {
			const core::TimeProviderPtr &timeProvider = _app->timeProvider();
			if (_newFolderName.name.empty()) {
				_newFolderError = TimedString(_("Folder name can't be empty"), timeProvider->tickNow(), 1500UL);
			} else {
				const core::String &newFilePath = assemblePath(_currentPath, _newFolderName);
				if (io::Filesystem::sysCreateDir(newFilePath)) {
					ImGui::CloseCurrentPopup();
				} else {
					_newFolderError = TimedString(_("Folder creation failed"), timeProvider->tickNow(), 1500UL);
				}
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::CancelButton()) {
			_newFolderName = io::FilesystemEntry();
			_newFolderError = TimedString();
			ImGui::CloseCurrentPopup();
		}
		showError(_newFolderError);
		ImGui::EndPopup();
	}
}

bool FileDialog::popupOptions(video::FileDialogOptions &fileDialogOptions_f, core::String &entityPath,
							  video::OpenFileMode type, const io::FormatDescription **formatDesc) {
	const core::String title = makeTitle(_("Options"), OPTIONS_POPUP);
	if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		const core::String &path = assemblePath(_currentPath, _selectedEntry);
		if (!fileDialogOptions_f || !fileDialogOptions_f(type, _currentFilterFormat, _selectedEntry) ||
			ImGui::OkButton()) {
			entityPath = path;
			resetState();
			*formatDesc = _currentFilterFormat;
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return true;
		}
		ImGui::TooltipTextUnformatted(path.c_str());
		ImGui::SameLine();
		if (ImGui::CancelButton()) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	return false;
}

bool FileDialog::popupAlreadyExists() {
	const core::String title = makeTitle(_("File already exists"), FILE_ALREADY_EXISTS_POPUP);
	if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::AlignTextToFramePadding();
		ImGui::PushFont(nullptr, imguiApp()->bigFontSize());
		ImGui::TextUnformatted(ICON_LC_TRIANGLE_ALERT);
		ImGui::PopFont();
		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();
		ImGui::Text(_("%s already exist.\nDo you want to overwrite the file?"), _selectedEntry.name.c_str());
		ImGui::Spacing();
		ImGui::Separator();

		if (ImGui::YesButton()) {
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return true;
		}
		ImGui::SameLine();
		if (ImGui::NoButton()) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	return false;
}

void FileDialog::filter(video::OpenFileMode type) {
	if (!_filterEntries.empty()) {
		ImGui::SameLine();
		const char *label = _("Filter");
		const ImVec2 &size = ImGui::CalcTextSize(label);
		const ImVec2 available = ImGui::GetContentRegionAvail();
		const float contentRegionWidth = available.x + ImGui::GetCursorPosX();
		ImGui::SetCursorPosX(contentRegionWidth - _filterTextWidth - ImGui::GetScrollX() - size.x - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::PushItemWidth(_filterTextWidth);
		int currentlySelected = _currentFilterEntry == -1 ? 0 : _currentFilterEntry;
		const core::String &selectedEntry = _currentFilterFormat ? io::convertToFilePattern(*_currentFilterFormat) : "";

		if (ImGui::BeginCombo(label, selectedEntry.c_str(), ImGuiComboFlags_HeightLargest)) {
			for (int i = 0; i < (int)_filterEntries.size(); ++i) {
				const bool selected = i == currentlySelected;
				const io::FormatDescription &format = _filterEntries[i];
				const core::String &text = io::convertToFilePattern(format);
				if (ImGui::Selectable(text.c_str(), selected)) {
					selectFilter(type, i);
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

void FileDialog::showError(const TimedString &error) const {
	const core::TimeProviderPtr &timeProvider = _app->timeProvider();
	if (error.isValid(timeProvider->tickNow())) {
		ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "%s", error.value().c_str());
	} else {
		ImGui::TextUnformatted("");
	}
}

const char *FileDialog::popupTitle(video::OpenFileMode type) {
	switch (type) {
	case video::OpenFileMode::Save:
		return _("Save file");
	case video::OpenFileMode::Directory:
		return _("Select a directory");
	case video::OpenFileMode::Open:
	default:
		break;
	}
	return _("Select a file");
}

void FileDialog::onTextInput(void *windowHandle, const core::String &text) {
	if (text.empty()) {
		return;
	}
	if (!_acceptInput) {
		return;
	}

	const core::TimeProviderPtr &timeProvider = _app->timeProvider();
	if (!_scrollToText.isValid(timeProvider->tickNow())) {
		_scrollToText.value().clear();
		_scrollToText = TimedString(text, timeProvider->tickNow(), 1500UL);
	} else {
		_scrollToText.value().append(text);
	}

	int idx = 0;
	for (const auto &entry : _filteredEntities) {
		if (core::string::startsWith(entry->name, _scrollToText.value())) {
			_selectedEntry = *entry;
			_entryIndex = idx;
			_scrollToSelection = true;
			break;
		}
		++idx;
	}
}

bool FileDialog::showFileDialog(video::FileDialogOptions &options, core::String &entityPath, video::OpenFileMode type,
								const io::FormatDescription **formatDesc, bool &showFileDialog) {
	_acceptInput = false;
	if (!showFileDialog) {
		return false;
	}
	float width = core_min(100.0f * ImGui::GetFontSize(), ImGui::GetMainViewport()->Size.x * 0.95f);
	const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
	ImGui::SetNextWindowSize(ImVec2(width, 0.0f));
	const char *title = popupTitle(type);
	if (!ImGui::IsPopupOpen(title)) {
		ImGui::OpenPopup(title);
		Log::debug("Opened popup %s", title);
	}

	if (ImGui::BeginPopupModal(title)) {
		_acceptInput = ImGui::GetTopMostPopupModal() == ImGui::GetCurrentWindow();
		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			showFileDialog = false;
			return false;
		}
		bool openSelectedEntry = false;
		currentPathPanel(type);
		openSelectedEntry |= quickAccessPanel(type, _bookmarks->strVal(), 20 * itemHeight);
		ImGui::SameLine();
		openSelectedEntry |= entitiesPanel(type, 20 * itemHeight);
		if (type != video::OpenFileMode::Open) {
			if (ImGui::Button(_("New folder"))) {
				ImGui::OpenPopup(NEW_FOLDER_POPUP);
			}
			ImGui::SameLine();
		}
		if (type == video::OpenFileMode::Save) {
			if (ImGui::InputText(_("Filename"), &_selectedEntry.name)) {
				_selectedEntry.fullPath =
					core::string::path(core::string::extractDir(_selectedEntry.fullPath), _selectedEntry.name);
			}
			_selectedEntry.type = io::FilesystemEntry::Type::file;
			_entryIndex = -1;
		}
		if (ImGui::CheckboxVar(_("Show hidden"), _showHidden)) {
			applyFilter(type);
		}
		popupNewFolder();
		popupNotWriteable();
		if (popupAlreadyExists()) {
			ImGui::OpenPopup(OPTIONS_POPUP);
		}
		if (popupOptions(options, entityPath, type, formatDesc)) {
			ImGui::EndPopup();
			return true;
		}
		filter(type);
		if (buttons(entityPath, type, openSelectedEntry)) {
			*formatDesc = _currentFilterFormat;
			ImGui::EndPopup();
			return true;
		}
		showError(_error);
		ImGui::EndPopup();
	}
	return false;
}

bool FileDialog::buttons(core::String &entityPath, video::OpenFileMode type, bool openSelectedEntry) {
	const char *buttonText = _("Choose");
	if (type == video::OpenFileMode::Open) {
		buttonText = _("Open");
	} else if (type == video::OpenFileMode::Save) {
		buttonText = _("Save");
	}

	if (ImGui::CancelButton()) {
		resetState();
		return true;
	}
	ImGui::SameLine();
	const core::TimeProviderPtr &timeProvider = _app->timeProvider();
	if (ImGui::Button(buttonText) || ImGui::IsKeyDown(ImGuiKey_Enter) || openSelectedEntry) {
		if (type == video::OpenFileMode::Directory) {
			if (_selectedEntry.name.empty()) {
				_error = TimedString(_("Error: You must select a folder!"), timeProvider->tickNow(), 1500UL);
			} else {
				entityPath = assemblePath(_currentPath, _selectedEntry);
				resetState();
				return true;
			}
		} else if (type == video::OpenFileMode::Open || type == video::OpenFileMode::Save) {
			if (_selectedEntry.name.empty() || !_selectedEntry.isFile()) {
				_error = TimedString(_("Error: You must select a file!"), timeProvider->tickNow(), 1500UL);
			} else {
				core::String fullPath = assemblePath(_currentPath, _selectedEntry);
				if (type == video::OpenFileMode::Save) {
					const core::String ext = core::string::extractExtension(fullPath);
					if (ext.empty()) {
						if (_currentFilterFormat == nullptr || _currentFilterFormat->mainExtension().empty()) {
							// if we didn't provide an extension, and we can't add one, we can't save the file
							_error = TimedString(_("Error: You must select a file type!"), timeProvider->tickNow(), 1500UL);
							return false;
						}
						fullPath.append(_currentFilterFormat->mainExtension(true));
					}
				}
				if (type == video::OpenFileMode::Save && io::Filesystem::sysExists(fullPath)) {
					ImGui::OpenPopup(FILE_ALREADY_EXISTS_POPUP);
				} else if (type == video::OpenFileMode::Save && !io::Filesystem::sysIsWriteable(fullPath)) {
					ImGui::OpenPopup(FILE_NOT_WRITEABLE_POPUP);
				} else {
					entityPath = fullPath;
					ImGui::OpenPopup(OPTIONS_POPUP);
				}
			}
		}
	}
	ImGui::SetItemDefaultFocus();
	return false;
}
} // namespace ui
