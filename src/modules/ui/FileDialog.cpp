/**
 * @file
 */

#include "FileDialog.h"
#include "IMGUIEx.h"
#include "IconsLucide.h"
#include "ScopedStyle.h"
#include "app/App.h"
#include "core/Algorithm.h"
#include "core/ArrayLength.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"
#include "io/FilesystemEntry.h"
#include "io/FormatDescription.h"
#ifdef __EMSCRIPTEN__
#include "io/MemoryReadStream.h"
#endif
#include "ui/IMGUIApp.h"
#include "video/OpenFileMode.h"

namespace ui {

// TODO: I18N
static const char *FILE_ALREADY_EXISTS_POPUP = "File already exists###fileoverwritepopup";
static const char *NEW_FOLDER_POPUP = "Create folder###newfolderpopup";
static const char *FILEDIALOGBOOKMARKDND = "filedialog-dir";

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
	{
		[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) { return lhs->name < rhs->name; },
		[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) { return lhs->name > rhs->name; }
	},
	{
		[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) { return lhs->size < rhs->size; },
		[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) { return lhs->size > rhs->size; }
	},
	{
		[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) {
			const core::String &aext = core::string::extractExtension(lhs->name);
			const core::String &bext = core::string::extractExtension(rhs->name);
			return aext < bext;
		},
		[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) {
			const core::String &aext = core::string::extractExtension(lhs->name);
			const core::String &bext = core::string::extractExtension(rhs->name);
			return aext > bext;
		}
	},
	{
		[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) { return lhs->mtime < rhs->mtime; },
		[](const io::FilesystemEntry *lhs, const io::FilesystemEntry *rhs) { return lhs->mtime > rhs->mtime; }
	}
};

static core::String assemblePath(const core::String &dir, const io::FilesystemEntry &ent) {
	return core::string::path(dir, ent.name);
}

void FileDialog::applyFilter(video::OpenFileMode type) {
	_files.clear();
	_files.reserve(_entities.size());
	for (size_t i = 0; i < _entities.size(); ++i) {
		if (_entities[i].type == io::FilesystemEntry::Type::dir) {
			_files.push_back(&_entities[i]);
			continue;
		} else if (type == video::OpenFileMode::Directory) {
			continue;
		}
		if (hide(_entities[i].name)) {
			continue;
		}
		if (_currentFilterEntry != -1) {
			// this is "all-supported files"
			const bool showAll = !_filterAll.empty() && _currentFilterEntry == 0;
			const core::String& filter = showAll ? _filterAll : _filterEntries[_currentFilterEntry].wildCard();
			if (!core::string::fileMatchesMultiple(_entities[i].name.c_str(), filter.c_str())) {
				continue;
			}
		}
		_files.push_back(&_entities[i]);
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

	if (_currentFilterEntry != -1 && type == video::OpenFileMode::Save) {
		const io::FormatDescription &desc = _filterEntries[_currentFilterEntry];
		const core::String &extension = core::string::extractExtension(_selectedEntry.name);
		if (!desc.exts.empty() && !desc.matchesExtension(extension)) {
			_selectedEntry.name = core::string::stripExtension(_selectedEntry.name);
			_selectedEntry.name.append(".");
			_selectedEntry.name.append(desc.exts[0]);
		}
	}
}

#ifdef __EMSCRIPTEN__
void FileDialog::uploadHandler(std::string const& filename, std::string const& mimetype, std::string_view buffer, void* userdata) {
	io::MemoryReadStream stream(buffer.data(), buffer.size());
	const core::String *path = (const core::String*)userdata;
	FileDialog *fileDialog = (FileDialog*)userdata;
	io::filesystem()->write(filename.c_str(), stream);
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
		core::sort(_filterEntries.begin(), _filterEntries.end(), core::Less<io::FormatDescription>());
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

	const core::String &filePath = core::string::extractPath(filename);
	if (filePath.empty() || !io::filesystem()->exists(filePath)) {
		const core::String &lastDir = _lastDirVar->strVal();
		if (io::filesystem()->exists(lastDir)) {
			_currentPath = lastDir;
		} else {
			_currentPath = io::filesystem()->homePath();
		}
	} else {
		_currentPath = filePath;
	}
	_selectedEntry = io::FilesystemEntry{core::string::extractFilenameWithExtension(filename), filename, io::FilesystemEntry::Type::file, 0, 0};
	_entryIndex = -1;

	if (!io::filesystem()->exists(_currentPath)) {
		_currentPath = io::filesystem()->homePath();
		_lastDirVar->setVal(_currentPath);
	}

#ifdef __EMSCRIPTEN__
	if (type == video::OpenFileMode::Open) {
		emscripten_browser_file::upload("", uploadHandler, &_currentPath);
	}
#endif

	return readDir(type);
}

bool FileDialog::readDir(video::OpenFileMode type) {
	_type = type;
	_entities.clear();
	if (!io::filesystem()->list(_currentPath, _entities)) {
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
	core::String bookmarkTitle;
	if (title == nullptr) {
		bookmarkTitle = path;
		if (bookmarkTitle.last() == '/') {
			bookmarkTitle.erase(bookmarkTitle.size() - 1);
		}
		const size_t pos = bookmarkTitle.rfind("/");
		if (pos != core::String::npos) {
			bookmarkTitle = bookmarkTitle.substr(pos + 1);
		}
	} else {
		bookmarkTitle = title;
	}
	bookmarkTitle += core::string::format("###%i", index);
	if (icon != nullptr) {
		const float x = ImGui::GetCursorPosX();
		ImGui::TextUnformatted(icon);
		ImGui::SameLine();
		ImGui::SetCursorPosX(x + 2.0f * ImGui::GetStyle().ItemInnerSpacing.x + 1.5f * (float)imguiApp()->fontSize());
	}
	const ImVec2 size(width, 0);
	if (ImGui::Selectable(bookmarkTitle.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, size)) {
		setCurrentPath(type, path);
	}
	ImGui::TooltipText("%s", path.c_str());
	return true;
}

void FileDialog::removeBookmark(const core::String &bookmark) {
	core::VarPtr bookmarks = core::Var::getSafe(cfg::UIBookmarks);

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

void FileDialog::quickAccessPanel(video::OpenFileMode type, const core::String &bookmarks, int height) {
	ScopedStyle style;
	style.setItemSpacing(ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()));
	const float width = ImGui::CalcTextSize("#").x * 20.0f;
	ImGui::BeginChild("bookmarks_child", ImVec2(width, height), ImGuiChildFlags_Border);
	const float contentRegionWidth = ImGui::GetWindowContentRegionMax().x;

	const char *folderNames[] = {_("Download"), _("Desktop"), _("Documents"), _("Pictures"), _("Public"), _("Recent"), _("Cloud")};
	static const char *folderIcons[] = {ICON_LC_DOWNLOAD, ICON_LC_MONITOR_DOT, ICON_LC_FILE, ICON_LC_IMAGE, ICON_LC_FOLDER, ICON_LC_FOLDER, ICON_LC_CLOUD};
	static_assert(lengthof(folderNames) == io::FilesystemDirectories::FS_Dir_Max, "Array size doesn't match enum value");
	static_assert(lengthof(folderIcons) == io::FilesystemDirectories::FS_Dir_Max, "Array size doesn't match enum value");

	if (ImGui::TreeNode(_("Quick Access"))) {
		for (int n = 0; n < io::FilesystemDirectories::FS_Dir_Max; ++n) {
			const core::String& dir = io::filesystem()->specialDir((io::FilesystemDirectories)n);
			if (dir.empty()) {
				continue;
			}
			quickAccessEntry(n, type, dir, contentRegionWidth, folderNames[n], folderIcons[n]);
		}
		int i = 0;
		for (const core::String &path : io::filesystem()->otherPaths()) {
			quickAccessEntry(i++, type, path, contentRegionWidth);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode(_("This PC"))) {
		const io::Paths& paths = io::filesystem()->paths();
		int i = 0;
		for (const core::String& path : paths) {
			const core::String& absPath = io::filesystem()->absolutePath(path);
			if (absPath.empty()) {
				continue;
			}
			quickAccessEntry(i++, type, absPath, contentRegionWidth, nullptr, ICON_LC_FOLDER);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode(_("Bookmarks"))) {
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(FILEDIALOGBOOKMARKDND)) {
				const core::String &directory = *(core::String *)payload->Data;
				addBookmark(directory);
			}
			ImGui::EndDragDropTarget();
		}
		core::DynamicArray<core::String> bm;
		core::string::splitString(bookmarks, bm, ";");
		int i = 0;
		for (const core::String& path : bm) {
			const core::String& absPath = io::filesystem()->absolutePath(path);
			if (absPath.empty()) {
				removeBookmark(path);
				continue;
			}
			if (quickAccessEntry(i++, type, absPath, contentRegionWidth, nullptr, ICON_LC_FOLDER)) {
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
}

void FileDialog::setCurrentPath(video::OpenFileMode type, const core::String& path) {
	resetState();
	_currentPath = path;
	core::Var::getSafe(cfg::UILastDirectory)->setVal(_currentPath);
	readDir(type);
}

bool FileDialog::hide(const core::String &file) const {
	if (_showHidden->boolVal()) {
		return false;
	}
	return file[0] == '.';
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
	ImGui::BeginChild("files", childSize, ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar);

	bool doubleClickedFile = false;
	bool doubleClickedDir = false;
	static const uint32_t TableFlags =
		ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable |
		ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable;
	const ImVec2 outerSize = ImGui::GetContentRegionAvail();
	if (ImGui::BeginTable("##files", 4, TableFlags, outerSize)) {
		ImGui::TableSetupColumn(_("File"), ImGuiTableColumnFlags_WidthStretch, 0.7f, (int)FileDialogColumnId::File);
		ImGui::TableSetupColumn(_("Size"), ImGuiTableColumnFlags_WidthStretch, 0.09f, (int)FileDialogColumnId::Size);
		ImGui::TableSetupColumn(_("Type"), ImGuiTableColumnFlags_WidthStretch, 0.07f, (int)FileDialogColumnId::Type);
		ImGui::TableSetupColumn(_("Date"), ImGuiTableColumnFlags_WidthStretch, 0.14f, (int)FileDialogColumnId::Date);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		// Sort files
		if (ImGuiTableSortSpecs *specs = ImGui::TableGetSortSpecs()) {
			if (specs->SpecsDirty && _files.size() > 1U) {
				for (int n = 0; n < specs->SpecsCount; n++) {
					const ImGuiTableColumnSortSpecs &spec = specs->Specs[n];
					if (spec.SortDirection == ImGuiSortDirection_Ascending) {
						_files.sort(fileDialogSorter[spec.ColumnUserID].asc);
					} else {
						_files.sort(fileDialogSorter[spec.ColumnUserID].desc);
					}
				}
				specs->SpecsDirty = false;
			}
		}

		const ImVec2 size(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x, 0);

		// add parent directory
		if (!core::string::isRootPath(_currentPath)) {
			ImGui::TableNextColumn();
			if (ImGui::Selectable("..", false, ImGuiSelectableFlags_AllowDoubleClick, size)) {
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					setCurrentPath(type, io::filesystem()->absolutePath(core::string::path(_currentPath, "..")));
				}
			}
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
		}

		// add filtered and sorted directory entries
		ImGuiListClipper clipper;
		clipper.Begin((int)_files.size(), ImGui::GetTextLineHeightWithSpacing());
		while (clipper.Step()) {
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
				const io::FilesystemEntry entry = *_files[i];
				ImGui::TableNextColumn();
				const bool selected = i == (int)_entryIndex;
				if (selected) {
					_selectedEntry = *_files[i];
				}
				const char *icon = iconForType(entry.type);
				const float x = ImGui::GetCursorPosX();
				ImGui::TextUnformatted(icon);
				ImGui::SameLine();
				ImGui::SetCursorPosX(x + 1.5f * (float)imguiApp()->fontSize());
				if (ImGui::Selectable(entry.name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick, size)) {
					resetState();
					_entryIndex = i;
					_selectedEntry = *_files[i];
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						if (entry.isDirectory()) {
							doubleClickedDir = true;
						} else {
							doubleClickedFile = true;
						}
					}
				}
				if (entry.type == io::FilesystemEntry::Type::dir) {
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						_dragAndDropName = core::string::path(_currentPath, entry.name);
						ImGui::TextUnformatted(_dragAndDropName.c_str());
						ImGui::SetDragDropPayload(FILEDIALOGBOOKMARKDND, &_dragAndDropName, sizeof(core::String),
													ImGuiCond_Always);
						ImGui::EndDragDropSource();
					}
				}
				ImGui::TableNextColumn();
				const core::String &humanSize = core::string::humanSize(entry.size);
				ImGui::TextUnformatted(humanSize.c_str());
				ImGui::TableNextColumn();
				if (entry.isDirectory()) {
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
	ImGui::EndChild();

	if (doubleClickedDir) {
		setCurrentPath(type, assemblePath(_currentPath, _selectedEntry));
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

	core::DynamicArray<core::String> components;
	core::string::splitString(_currentPath, components, "/");
	ImGui::TextUnformatted(">");
	core::String path = "/";
#ifdef _WIN32
	path = "";
#endif
	for (const core::String &c : components) {
		path = core::string::sanitizeDirPath(core::string::path(path, c));
		ImGui::SameLine();
		if (ImGui::Button(c.c_str())) {
			setCurrentPath(type, path);
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			_dragAndDropName = path;
			ImGui::TextUnformatted(_dragAndDropName.c_str());
			ImGui::SetDragDropPayload(FILEDIALOGBOOKMARKDND, &_dragAndDropName, sizeof(core::String),
										ImGuiCond_Always);
			ImGui::EndDragDropSource();
		}
		ImGui::TooltipText("%s", path.c_str());
	}
}

void FileDialog::construct() {
	_bookmarks = core::Var::get(cfg::UIBookmarks, "");
	_showHidden = core::Var::get(cfg::UIFileDialogShowHidden, "false", _("Show hidden file system entities"));
	_lastDirVar = core::Var::get(cfg::UILastDirectory, io::filesystem()->homePath().c_str());
	_lastFilterSave = core::Var::get(cfg::UILastFilterSave, "0", _("The last selected file type filter in the file dialog"));
	_lastFilterOpen = core::Var::get(cfg::UILastFilterOpen, "0", _("The last selected file type filter in the file dialog"));
}

void FileDialog::resetState() {
	_entryIndex = 0;
	_selectedEntry = io::FilesystemEntry();
	_error = TimedError();
}

void FileDialog::popupNewFolder() {
	const ImVec2 &windowPos = ImGui::GetWindowPos();
	const ImVec2 &windowSize = ImGui::GetWindowSize();
	const ImVec2 center(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(NEW_FOLDER_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted(_("Enter a name for the new folder"));
		ImGui::InputText("##newfoldername", &_newFolderName.name);
		if (ImGui::Button(_("Create"))) {
			if (_newFolderName.name.empty()) {
				const core::TimeProviderPtr &timeProvider = app::App::getInstance()->timeProvider();
				_newFolderError = TimedError(_("Folder name can't be empty"), timeProvider->tickNow(), 1500UL);
			} else {
				const core::String &newFilePath = assemblePath(_currentPath, _newFolderName);
				io::filesystem()->createDir(newFilePath);
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button(_("Cancel")) || ImGui::IsKeyDown(ImGuiKey_Escape)) {
			_newFolderName = io::FilesystemEntry();
			_newFolderError = TimedError();
			ImGui::CloseCurrentPopup();
		}
		showError(_newFolderError);
		ImGui::EndPopup();
	}
}

bool FileDialog::popupAlreadyExists() {
	if (ImGui::BeginPopupModal(FILE_ALREADY_EXISTS_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::AlignTextToFramePadding();
		ImGui::PushFont(imguiApp()->bigFont());
		ImGui::TextUnformatted(ICON_LC_TRIANGLE_ALERT);
		ImGui::PopFont();
		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();
		ImGui::Text(_("%s already exist.\nDo you want to overwrite the file?"), _selectedEntry.name.c_str());
		ImGui::Spacing();
		ImGui::Separator();

		if (ImGui::Button(_("Yes"))) {
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return true;
		}
		ImGui::SameLine();
		if (ImGui::Button(_("No"))) {
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
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - _filterTextWidth - ImGui::GetScrollX() - size.x - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::PushItemWidth(_filterTextWidth);
		int currentlySelected = _currentFilterEntry == -1 ? 0 : _currentFilterEntry;
		const core::String &selectedEntry = io::convertToFilePattern(_filterEntries[currentlySelected]);

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

void FileDialog::showError(const TimedError &error) const {
	const core::TimeProviderPtr &timeProvider = app::App::getInstance()->timeProvider();
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

bool FileDialog::showFileDialog(video::FileDialogOptions &options, core::String &entityPath, video::OpenFileMode type, const io::FormatDescription **formatDesc) {
	float width = core_min(100.0f * ImGui::GetFontSize(), ImGui::GetMainViewport()->Size.x * 0.95f);
	const float itemHeight = (ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y);
	ImGui::SetNextWindowSize(ImVec2(width, 25 * itemHeight), ImGuiCond_FirstUseEver);
	const char *title = popupTitle(type);
	if (!ImGui::IsPopupOpen(title)) {
		ImGui::OpenPopup(title);
	}

	if (ImGui::BeginPopupModal(title)) {
		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			ImGui::CloseCurrentPopup();
		}
		currentPathPanel(type);
		quickAccessPanel(type, _bookmarks->strVal(), 20 * itemHeight);
		ImGui::SameLine();
		bool doubleClickedFile = entitiesPanel(type, 20 * itemHeight);
		if (type != video::OpenFileMode::Open) {
			if (ImGui::Button(_("New folder"))) {
				ImGui::OpenPopup(NEW_FOLDER_POPUP);
			}
			ImGui::SameLine();
		}
		if (type == video::OpenFileMode::Save) {
			ImGui::InputText(_("Filename"), &_selectedEntry.name);
			_selectedEntry.type = io::FilesystemEntry::Type::file;
			_entryIndex = -1;
		}
		if (ImGui::CheckboxVar(_("Show hidden"), _showHidden)) {
			applyFilter(type);
		}
		popupNewFolder();
		if (popupAlreadyExists()) {
			entityPath = assemblePath(_currentPath, _selectedEntry);
			resetState();
			*formatDesc = _currentFilterFormat;
			ImGui::EndPopup();
			return true;
		}
		filter(type);
		if (buttons(entityPath, type, doubleClickedFile)) {
			*formatDesc = _currentFilterFormat;
			ImGui::EndPopup();
			return true;
		}
		showError(_error);
		if (options && ImGui::CollapsingHeader(_("Options"))) {
			options(type, _currentFilterFormat);
		}
		ImGui::EndPopup();
	}
	return false;
}

bool FileDialog::buttons(core::String &entityPath, video::OpenFileMode type, bool doubleClickedFile) {
	const char *buttonText = _("Choose");
	if (type == video::OpenFileMode::Open) {
		buttonText = _("Open");
	} else if (type == video::OpenFileMode::Save) {
		buttonText = _("Save");
	}

	const ImVec2 cancelTextSize = ImGui::CalcTextSize(_("Cancel"));
	const ImVec2 chooseTextSize = ImGui::CalcTextSize(buttonText);
	ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - cancelTextSize.x - chooseTextSize.x - 40.0f);
	if (ImGui::Button(_("Cancel")) || ImGui::IsKeyDown(ImGuiKey_Escape)) {
		resetState();
		return true;
	}
	ImGui::SameLine();
	const core::TimeProviderPtr &timeProvider = app::App::getInstance()->timeProvider();
	if (ImGui::Button(buttonText) || ImGui::IsKeyDown(ImGuiKey_Enter) || doubleClickedFile) {
		if (type == video::OpenFileMode::Directory) {
			if (_selectedEntry.name.empty()) {
				_error = TimedError(_("Error: You must select a folder!"), timeProvider->tickNow(), 1500UL);
			} else {
				entityPath = assemblePath(_currentPath, _selectedEntry);
				resetState();
				return true;
			}
		} else if (type == video::OpenFileMode::Open || type == video::OpenFileMode::Save) {
			if (_selectedEntry.name.empty() || !_selectedEntry.isFile()) {
				_error = TimedError(_("Error: You must select a file!"), timeProvider->tickNow(), 1500UL);
			} else {
				const core::String &fullPath = assemblePath(_currentPath, _selectedEntry);
				if (type == video::OpenFileMode::Save && io::filesystem()->exists(fullPath)) {
					ImGui::OpenPopup(FILE_ALREADY_EXISTS_POPUP);
				} else {
					entityPath = fullPath;
					resetState();
					return true;
				}
			}
		}
	}
	ImGui::SetItemDefaultFocus();
	return false;
}

}
