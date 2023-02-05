/**
 * @file
 */

#include "FileDialog.h"
#include "IconsFontAwesome6.h"
#include "IconsForkAwesome.h"
#include "ScopedStyle.h"
#include "app/App.h"
#include "core/Algorithm.h"
#include "core/ArrayLength.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/Trace.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "dearimgui/imgui_internal.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIApp.h"

namespace ui {

static const char *FILE_ALREADY_EXISTS_POPUP = "File already exists##FileOverwritePopup";
static const char *NEW_FOLDER_POPUP = "Create folder##NewFolderPopup";

static core::String assemblePath(const core::String &dir, const core::String &ent) {
	return core::string::path(dir, ent);
}

void FileDialog::applyFilter() {
	_files.clear();
	_files.reserve(_entities.size());
	for (size_t i = 0; i < _entities.size(); ++i) {
		if (_entities[i].type != io::FilesystemEntry::Type::file) {
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

void FileDialog::selectFilter(int index) {
	core_assert(index >= -1 && index <= (int)_filterEntries.size());
	_currentFilterEntry = index;
	core::Var::getSafe(cfg::UILastFilter)->setVal(_currentFilterEntry);
	if (_currentFilterEntry != -1) {
		_currentFilterFormat = &_filterEntries[_currentFilterEntry];
	} else {
		_currentFilterFormat = nullptr;
	}
	applyFilter();
}

bool FileDialog::openDir(const io::FormatDescription* formats, const core::String& filename) {
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
		io::createGroupPatterns(formats, _filterEntries);
		_filterAll = io::convertToAllFilePattern(formats);
		if (!_filterAll.empty()) {
			// must be the first entry - see applyFilter()
			_filterEntries.insert(_filterEntries.begin(), io::ALL_SUPPORTED);
		}

		const core::VarPtr &lastFilterVar = core::Var::getSafe(cfg::UILastFilter);
		int lastFilter = lastFilterVar->intVal();
		if (lastFilter < 0 || lastFilter >= (int)_filterEntries.size()) {
			lastFilter = 0;
		}
		selectFilter(lastFilter);
	}

	const core::String &filePath = core::string::extractPath(filename);
	if (filePath.empty() || !io::filesystem()->exists(filePath)) {
		const core::VarPtr &lastDirVar = core::Var::getSafe(cfg::UILastDirectory);
		const core::String &lastDir = lastDirVar->strVal();
		_currentPath = lastDir;
	} else {
		_currentPath = filePath;
	}
	_currentFile = core::string::extractFilenameWithExtension(filename);
	_currentFolder = "";

	if (!io::filesystem()->exists(_currentPath)) {
		_currentPath = io::filesystem()->homePath();
		core::Var::getSafe(cfg::UILastDirectory)->setVal(_currentPath);
	}

	return readDir();
}

bool FileDialog::readDir() {
	_entities.clear();
	if (!io::filesystem()->list(_currentPath, _entities)) {
		Log::warn("Failed to list dir %s", _currentPath.c_str());
		return false;
	}

	applyFilter();
	return true;
}

void FileDialog::bookMarkEntry(video::OpenFileMode type, const core::String& path, float width, const char *title, const char *icon) {
	if (path.empty()) {
		return;
	}
	core::String bookmarkTitle;
	if (title == nullptr) {
		bookmarkTitle = path;
		if (bookmarkTitle.last() == '/') {
			bookmarkTitle.erase(bookmarkTitle.size() - 1);
		}
		title = SDL_strrchr(bookmarkTitle.c_str(), '/');
		if (title) {
			++title;
		} else {
			title = bookmarkTitle.c_str();
		}
		bookmarkTitle = title;
	} else {
		bookmarkTitle = title;
	}
	if (icon != nullptr) {
		const float x = ImGui::GetCursorPosX();
		ImGui::TextUnformatted(icon);
		ImGui::SameLine();
		ImGui::SetCursorPosX(x + 1.5f * (float)imguiApp()->fontSize());
	}
	const ImVec2 size(width, 0);
	if (ImGui::Selectable(bookmarkTitle.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, size)) {
		setCurrentPath(type, path);
	}
	ImGui::TooltipText("%s", path.c_str());
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

void FileDialog::bookmarkPanel(video::OpenFileMode type, const core::String &bookmarks) {
	ScopedStyle style;
	style.setItemSpacing(ImVec2(10, 10));
	ImGui::BeginChild("Bookmarks##filedialog", ImVec2(200, 300), true);
	const float contentRegionWidth = ImGui::GetWindowContentRegionMax().x;

	static const char *folderNames[] = {"Download", "Desktop", "Documents", "Pictures", "Public", "Recent", "Cloud"};
	static const char *folderIcons[] = {ICON_FK_DOWNLOAD, ICON_FK_DESKTOP, ICON_FA_FILE, ICON_FA_IMAGE, ICON_FA_FOLDER, ICON_FA_FOLDER, ICON_FA_CLOUD};
	static_assert(lengthof(folderNames) == io::FilesystemDirectories::FS_Dir_Max, "Array size doesn't match enum value");
	static_assert(lengthof(folderIcons) == io::FilesystemDirectories::FS_Dir_Max, "Array size doesn't match enum value");

	if (ImGui::TreeNode("Quick Access")) {
		for (int n = 0; n < io::FilesystemDirectories::FS_Dir_Max; ++n) {
			const core::String& dir = io::filesystem()->specialDir((io::FilesystemDirectories)n);
			if (dir.empty()) {
				continue;
			}
			bookMarkEntry(type, dir, contentRegionWidth, folderNames[n], folderIcons[n]);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("This PC")) {
		const io::Paths& paths = io::filesystem()->paths();
		for (const core::String& path : paths) {
			const core::String& absPath = io::filesystem()->absolutePath(path);
			if (absPath.empty()) {
				continue;
			}
			bookMarkEntry(type, absPath, contentRegionWidth, nullptr, ICON_FA_FOLDER);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Bookmarks")) {
		core::DynamicArray<core::String> bm;
		core::string::splitString(bookmarks, bm, ";");
		for (const core::String& path : bm) {
			const core::String& absPath = io::filesystem()->absolutePath(path);
			if (absPath.empty()) {
				removeBookmark(path);
				continue;
			}
			if (ImGui::Button(ICON_FK_TRASH)) {
				removeBookmark(path);
			}
			ImGui::TooltipText("Delete this bookmark");
			ImGui::SameLine();
			bookMarkEntry(type, absPath, contentRegionWidth, nullptr, ICON_FA_FOLDER);
		}
		ImGui::TreePop();
	}

	ImGui::EndChild();
}

void FileDialog::setCurrentPath(video::OpenFileMode type, const core::String& path) {
	if (type != video::OpenFileMode::Save) {
		_currentFile = "";
	}
	_folderSelectIndex = 0;
	_fileSelectIndex = 0;
	_currentFolder = "";
	_error[0] = '\0';
	_currentPath = path;
	core::Var::getSafe(cfg::UILastDirectory)->setVal(_currentPath);
	readDir();
}

void FileDialog::directoryPanel(video::OpenFileMode type) {
	ImGui::BeginChild("Directories##filedialog", ImVec2(200, 300), true,
					  ImGuiWindowFlags_HorizontalScrollbar);

	const float contentRegionWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
	if (ImGui::Selectable("..", false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(contentRegionWidth, 0))) {
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			setCurrentPath(type, io::filesystem()->absolutePath(_currentPath + "/.."));
		}
	}
	for (size_t i = 0; i < _entities.size(); ++i) {
		if (_entities[i].type != io::FilesystemEntry::Type::dir) {
			continue;
		}
		if (hide(_entities[i].name)) {
			continue;
		}
		const bool selected = i == _folderSelectIndex;
		const ImVec2 size(contentRegionWidth, 0);
		if (ImGui::Selectable(_entities[i].name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick, size)) {
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				setCurrentPath(type, assemblePath(_currentPath, _entities[i].name));
				break;
			} else {
				_folderSelectIndex = i;
				_currentFolder = _entities[i].name;
			}
		}
	}
	ImGui::EndChild();
}

bool FileDialog::hide(const core::String &file) const {
	if (_showHidden->boolVal()) {
		return false;
	}
	return file[0] == '.';
}

bool FileDialog::filesPanel() {
	ImGui::BeginChild("Files##1", ImVec2(ImGui::GetContentRegionAvail().x, 300), true,
					  ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::Columns(4);
	static float initialSpacingColumn3 = 120.0f;
	if (initialSpacingColumn3 > 0) {
		ImGui::SetColumnWidth(3, initialSpacingColumn3);
		initialSpacingColumn3 = 0.0f;
	}
	static float initialSpacingColumn1 = 80.0f;
	if (initialSpacingColumn1 > 0) {
		ImGui::SetColumnWidth(1, initialSpacingColumn1);
		initialSpacingColumn1 = 0.0f;
	}
	static float initialSpacingColumn2 = 80.0f;
	if (initialSpacingColumn2 > 0) {
		ImGui::SetColumnWidth(2, initialSpacingColumn2);
		initialSpacingColumn2 = 0.0f;
	}
	if (ImGui::Selectable("File##filespanel")) {
		_sizeSortOrder = FileDialogSortOrder::None;
		_dateSortOrder = FileDialogSortOrder::None;
		_typeSortOrder = FileDialogSortOrder::None;
		_fileNameSortOrder =
			(_fileNameSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
	}
	ImGui::NextColumn();
	if (ImGui::Selectable("Size##filespanel")) {
		_fileNameSortOrder = FileDialogSortOrder::None;
		_dateSortOrder = FileDialogSortOrder::None;
		_typeSortOrder = FileDialogSortOrder::None;
		_sizeSortOrder =
			(_sizeSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
	}
	ImGui::NextColumn();
	if (ImGui::Selectable("Type##filespanel")) {
		_fileNameSortOrder = FileDialogSortOrder::None;
		_dateSortOrder = FileDialogSortOrder::None;
		_sizeSortOrder = FileDialogSortOrder::None;
		_typeSortOrder =
			(_typeSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
	}
	ImGui::NextColumn();
	if (ImGui::Selectable("Date##filespanel")) {
		_fileNameSortOrder = FileDialogSortOrder::None;
		_sizeSortOrder = FileDialogSortOrder::None;
		_typeSortOrder = FileDialogSortOrder::None;
		_dateSortOrder =
			(_dateSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
	}
	ImGui::NextColumn();
	ImGui::Separator();

	static auto nameSorter = [this](const io::FilesystemEntry *a, const io::FilesystemEntry *b) {
		if (_fileNameSortOrder == FileDialogSortOrder::Down) {
			return a->name < b->name;
		}
		return a->name > b->name;
	};

	static auto sizeSorter = [this](const io::FilesystemEntry *a, const io::FilesystemEntry *b) {
		if (_sizeSortOrder == FileDialogSortOrder::Down) {
			return a->size < b->size;
		}
		return a->size > b->size;
	};

	static auto extensionSorter = [this](const io::FilesystemEntry *a, const io::FilesystemEntry *b) {
		const core::String &aext = core::string::extractExtension(a->name);
		const core::String &bext = core::string::extractExtension(b->name);
		if (_typeSortOrder == FileDialogSortOrder::Down) {
			return aext < bext;
		}
		return aext > bext;
	};

	static auto mtimeSorter = [this](const io::FilesystemEntry *a, const io::FilesystemEntry *b) {
		if (_dateSortOrder == FileDialogSortOrder::Down) {
			return a->mtime < b->mtime;
		}
		return a->mtime > b->mtime;
	};

	// Sort files
	if (_fileNameSortOrder != FileDialogSortOrder::None) {
		_files.sort(nameSorter);
	} else if (_sizeSortOrder != FileDialogSortOrder::None) {
		_files.sort(sizeSorter);
	} else if (_typeSortOrder != FileDialogSortOrder::None) {
		_files.sort(extensionSorter);
	} else if (_dateSortOrder != FileDialogSortOrder::None) {
		_files.sort(mtimeSorter);
	}

	bool doubleClicked = false;
	const ImVec2 size(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x, 0);
	for (size_t i = 0; i < _files.size(); ++i) {
		if (hide(_files[i]->name)) {
			continue;
		}
		const bool selected = i == _fileSelectIndex;
		if (ImGui::Selectable(_files[i]->name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick, size)) {
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				doubleClicked = true;
			}
			_fileSelectIndex = i;
			_currentFile = _files[i]->name;
			_currentFolder = "";
			_error[0] = '\0';
		}
		ImGui::NextColumn();
		const core::String &humanSize = core::string::humanSize(_files[i]->size);
		ImGui::TextUnformatted(humanSize.c_str());
		ImGui::NextColumn();
		const core::String &fileExt = core::string::extractExtension(_files[i]->name);
		if (fileExt.empty()) {
			ImGui::TextUnformatted("-");
		} else {
			ImGui::TextUnformatted(fileExt.c_str());
		}
		ImGui::NextColumn();
		const core::String &lastModified = core::TimeProvider::toString(_files[i]->mtime);
		ImGui::TextUnformatted(lastModified.c_str());
		ImGui::NextColumn();
	}
	ImGui::EndChild();
	return doubleClicked;
}

bool FileDialog::showFileDialog(bool *open, video::FileDialogOptions &fileDialogOptions, char *buffer, unsigned int bufferSize,
								video::OpenFileMode type, const io::FormatDescription **formatDesc) {
	if (open == nullptr || *open) {
		bool doubleClickedFile = false;
		core_trace_scoped(FileDialog);
		// TODO: not dpi aware
		ImGui::SetNextWindowSize(ImVec2(1200.0f, 700.0f), ImGuiCond_FirstUseEver);
		const char *title;
		switch (type) {
		case video::OpenFileMode::Save:
			title = "Save file";
			break;
		case video::OpenFileMode::Directory:
			title = "Select a directory";
			break;
		case video::OpenFileMode::Open:
		default:
			title = "Select a file";
			break;
		}
		if (!ImGui::IsPopupOpen(title)) {
			ImGui::OpenPopup(title);
		}
		_showHidden = core::Var::getSafe(cfg::UIFileDialogShowHidden);
		if (ImGui::BeginPopupModal(title, open)) {
			if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				ImGui::CloseCurrentPopup();
			}

			core::VarPtr bookmarks = core::Var::getSafe(cfg::UIBookmarks);
			if (ImGui::Button(ICON_FK_BOOKMARK)) {
				removeBookmark(_currentPath);
				core::String bm = bookmarks->strVal();
				if (bm.empty()) {
					bm.append(_currentPath);
				} else {
					bm.append(";" + _currentPath);
				}
				bookmarks->setVal(bm);
			}
			ImGui::TooltipText("Add a bookmark for the current active folder");
			ImGui::SameLine();
			const core::String currentPath = core::string::format(ICON_FK_FOLDER_OPEN_O " Current path: %s", _currentPath.c_str());
			ImGui::TextUnformatted(currentPath.c_str());

			bookmarkPanel(type, bookmarks->strVal());

			ImGui::SameLine();

			directoryPanel(type);

			ImGui::SameLine();

			doubleClickedFile = filesPanel();

			core::String selectedFilePath =
				assemblePath(_currentPath, !_currentFolder.empty() ? _currentFolder : _currentFile);
			ImGui::PushItemWidth(724);
			ImGui::InputText("##selectedpath", &selectedFilePath, ImGuiInputTextFlags_ReadOnly);
			if (type == video::OpenFileMode::Save) {
				ImGui::InputText("Filename", &_currentFile);
			}
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

			if (type != video::OpenFileMode::Open) {
				if (ImGui::Button("New folder")) {
					ImGui::OpenPopup(NEW_FOLDER_POPUP);
				}
				ImGui::SameLine();
			}

			ImGui::CheckboxVar("Show hidden", _showHidden);

			ImVec2 center(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x * 0.5f,
						  ImGui::GetWindowPos().y + ImGui::GetWindowSize().y * 0.5f);
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal(NEW_FOLDER_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Enter a name for the new folder");
				ImGui::InputText("##newfoldername", _newFolderName, sizeof(_newFolderName));
				if (ImGui::Button("Create##1")) {
					if (strlen(_newFolderName) <= 0) {
						SDL_strlcpy(_newFolderError, "Folder name can't be empty", sizeof(_newFolderError));
					} else {
						const core::String &newFilePath = assemblePath(_currentPath, _newFolderName);
						io::filesystem()->createDir(newFilePath);
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("Cancel##1") || ImGui::IsKeyDown(ImGuiKey_Escape)) {
					_newFolderName[0] = '\0';
					_newFolderError[0] = '\0';
					ImGui::CloseCurrentPopup();
				}
				ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "%s", _newFolderError);
				ImGui::EndPopup();
			}

			if (ImGui::BeginPopupModal(FILE_ALREADY_EXISTS_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::AlignTextToFramePadding();
				ImGui::PushFont(imguiApp()->bigFont());
				ImGui::TextUnformatted(ICON_FA_TRIANGLE_EXCLAMATION);
				ImGui::PopFont();
				ImGui::SameLine();
				ImGui::Spacing();
				ImGui::SameLine();
				ImGui::TextUnformatted("Do you want to overwrite the file?");
				ImGui::Spacing();
				ImGui::Separator();

				if (ImGui::Button("Yes##filedialog-override")) {
					const core::String &fullPath = assemblePath(_currentPath, _currentFile);
					SDL_strlcpy(buffer, fullPath.c_str(), bufferSize);
					_fileSelectIndex = 0;
					_folderSelectIndex = 0;
					_currentFile = "";
					if (open != nullptr) {
						*open = false;
					}
					if (formatDesc != nullptr) {
						*formatDesc = _currentFilterFormat;
					}
					_error[0] = '\0';
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					ImGui::EndPopup();
					return true;
				}
				ImGui::SameLine();
				if (ImGui::Button("No##filedialog-override")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			if (!_filterEntries.empty()) {
				ImGui::SameLine();
				const char *label = "Filter";
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
							selectFilter(i);
						}
						if (selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();
			}

			const char *buttonText = "Choose";
			if (type == video::OpenFileMode::Open) {
				buttonText = "Open";
			} else if (type == video::OpenFileMode::Save) {
				buttonText = "Save";
			}

			const ImVec2 cancelTextSize = ImGui::CalcTextSize("Cancel");
			const ImVec2 chooseTextSize = ImGui::CalcTextSize(buttonText);
			ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - cancelTextSize.x - chooseTextSize.x - 40.0f);
			if (ImGui::Button("Cancel") || ImGui::IsKeyDown(ImGuiKey_Escape)) {
				_fileSelectIndex = 0;
				_folderSelectIndex = 0;
				_currentFile = "";
				core_assert(bufferSize >= 1);
				buffer[0] = '\0';
				if (open != nullptr) {
					*open = false;
				}
				if (formatDesc != nullptr) {
					*formatDesc = _currentFilterFormat;
				}
				ImGui::EndPopup();
				return true;
			}
			ImGui::SameLine();
			if (ImGui::Button(buttonText) || ImGui::IsKeyDown(ImGuiKey_Enter) || doubleClickedFile) {
				if (type == video::OpenFileMode::Directory) {
					if (_currentFolder == "") {
						SDL_strlcpy(_error, "Error: You must select a folder!", sizeof(_error));
					} else {
						const core::String &fullPath = assemblePath(_currentPath, _currentFolder);
						SDL_strlcpy(buffer, fullPath.c_str(), bufferSize);
						_fileSelectIndex = 0;
						_folderSelectIndex = 0;
						_currentFile = "";
						if (open != nullptr) {
							*open = false;
						}
						if (formatDesc != nullptr) {
							*formatDesc = _currentFilterFormat;
						}
						_error[0] = '\0';
						ImGui::EndPopup();
						return true;
					}
				} else if (type == video::OpenFileMode::Open ||
						   type == video::OpenFileMode::Save) {
					if (_currentFile == "") {
						SDL_strlcpy(_error, "Error: You must select a file!", sizeof(_error));
					} else {
						if (_currentFilterEntry != -1 && core::string::extractExtension(_currentFile).empty()) {
							const io::FormatDescription &desc = _filterEntries[_currentFilterEntry];
							if (!desc.exts[0].empty()) {
								_currentFile.append(".");
								_currentFile.append(desc.exts[0]);
							}
						}
						const core::String &fullPath = assemblePath(_currentPath, _currentFile);
						if (type == video::OpenFileMode::Save && io::filesystem()->exists(fullPath)) {
							ImGui::OpenPopup(FILE_ALREADY_EXISTS_POPUP);
						} else {
							SDL_strlcpy(buffer, fullPath.c_str(), bufferSize);
							_fileSelectIndex = 0;
							_folderSelectIndex = 0;
							_currentFile = "";
							if (open != nullptr) {
								*open = false;
							}
							if (formatDesc != nullptr) {
								*formatDesc = _currentFilterFormat;
							}
							_error[0] = '\0';
							ImGui::EndPopup();
							return true;
						}
					}
				}
			}
			ImGui::SetItemDefaultFocus();

			if (strlen(_error) > 0) {
				ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "%s", _error);
			} else {
				ImGui::Spacing();
			}

			// TODO: allow to hide them
			if (fileDialogOptions) {
				fileDialogOptions(type, _currentFilterFormat);
			}

			ImGui::EndPopup();
		}
	}
	return false;
}

}
