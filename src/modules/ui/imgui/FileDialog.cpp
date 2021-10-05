/**
 * @file
 */

#include "FileDialog.h"
#include "IMGUI.h"
#include "app/App.h"
#include "core/Algorithm.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/Trace.h"
#include "core/Var.h"
#include "io/Filesystem.h"

namespace ui {
namespace imgui {

static core::String assemblePath(const core::String &dir, const core::String &ent) {
	return dir + (dir.last() == '/' ? "" : "/") + ent;
}

void FileDialog::applyFilter() {
	files.clear();
	files.reserve(entities.size());
	for (size_t i = 0; i < entities.size(); ++i) {
		if (entities[i].type != io::Filesystem::DirEntry::Type::file) {
			continue;
		}
		if (!_filter.empty()) {
			if (!core::string::fileMatchesMultiple(entities[i].name.c_str(), _filter.c_str())) {
				continue;
			}
		}
		files.push_back(&entities[i]);
	}
}

bool FileDialog::openDir(const core::String& filter) {
	_filter = filter;

	const core::VarPtr& lastDirVar = core::Var::getSafe(cfg::UILastDirectory);
	const core::String& lastDir = lastDirVar->strVal();
	fileDialogCurrentPath = lastDir;
	fileDialogCurrentFile = "";
	fileDialogCurrentFolder = "";

	if (!io::filesystem()->exists(fileDialogCurrentPath)) {
		fileDialogCurrentPath = io::filesystem()->homePath();
		core::Var::getSafe(cfg::UILastDirectory)->setVal(fileDialogCurrentPath);
	}

	return readDir();
}

bool FileDialog::readDir() {
	entities.clear();
	if (!io::filesystem()->list(fileDialogCurrentPath, entities)) {
		Log::warn("Failed to list dir %s", fileDialogCurrentPath.c_str());
		return false;
	}

	applyFilter();
	return true;
}

// TODO: make filters selectable
// TODO: list registered data paths from filesystem
// TODO: allow to specify the starting directory
bool FileDialog::showFileDialog(bool *open, char *buffer, unsigned int bufferSize, video::WindowedApp::OpenFileMode type) {
	if (open == nullptr || *open) {
		core_trace_scoped(FileDialog);
		ImGui::SetNextWindowSize(ImVec2(ImGui::Size(740.0f), ImGui::Size(494.0f)), ImGuiCond_FirstUseEver);
		const char *title;
		switch (type){
		case video::WindowedApp::OpenFileMode::Directory:
			title = "Select a directory";
			break;
		case video::WindowedApp::OpenFileMode::Save:
		case video::WindowedApp::OpenFileMode::Open:
		default:
			title = "Select a file";
			break;
		}
		ImGui::Begin(title, nullptr);

		ImGui::Text("%s", fileDialogCurrentPath.c_str());

		ImGui::BeginChild("Directories##1", ImVec2(ImGui::Size(200), ImGui::Size(300)), true, ImGuiWindowFlags_HorizontalScrollbar);

		const float contentRegionWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
		if (ImGui::Selectable("..", false, ImGuiSelectableFlags_AllowDoubleClick,
							  ImVec2(contentRegionWidth, 0))) {
			if (ImGui::IsMouseDoubleClicked(0)) {
				fileDialogCurrentPath = io::filesystem()->absolutePath(fileDialogCurrentPath + "/..");
				core::Var::getSafe(cfg::UILastDirectory)->setVal(fileDialogCurrentPath);
				readDir();
			}
		}
		for (size_t i = 0; i < entities.size(); ++i) {
			if (entities[i].type != io::Filesystem::DirEntry::Type::dir) {
				continue;
			}
			if (ImGui::Selectable(entities[i].name.c_str(), i == fileDialogFolderSelectIndex,
								  ImGuiSelectableFlags_AllowDoubleClick,
								  ImVec2(contentRegionWidth, 0))) {
				fileDialogCurrentFile = "";
				if (ImGui::IsMouseDoubleClicked(0)) {
					fileDialogCurrentPath = assemblePath(fileDialogCurrentPath, entities[i].name);
					core::Var::getSafe(cfg::UILastDirectory)->setVal(fileDialogCurrentPath);
					fileDialogFolderSelectIndex = 0;
					fileDialogFileSelectIndex = 0;
					ImGui::SetScrollHereY(0.0f);
					fileDialogCurrentFolder = "";
					fileDialogError[0] = '\0';
					readDir();
					break;
				} else {
					fileDialogFolderSelectIndex = i;
					fileDialogCurrentFolder = entities[i].name;
				}
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("Files##1", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::Size(300)), true, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::Columns(4);
		static float initialSpacingColumn3 = ImGui::Size(120.0f);
		if (initialSpacingColumn3 > 0) {
			ImGui::SetColumnWidth(3, initialSpacingColumn3);
			initialSpacingColumn3 = 0.0f;
		}
		static float initialSpacingColumn1 = ImGui::Size(80.0f);
		if (initialSpacingColumn1 > 0) {
			ImGui::SetColumnWidth(1, initialSpacingColumn1);
			initialSpacingColumn1 = 0.0f;
		}
		static float initialSpacingColumn2 = ImGui::Size(80.0f);
		if (initialSpacingColumn2 > 0) {
			ImGui::SetColumnWidth(2, initialSpacingColumn2);
			initialSpacingColumn2 = 0.0f;
		}
		if (ImGui::Selectable("File")) {
			sizeSortOrder = FileDialogSortOrder::None;
			dateSortOrder = FileDialogSortOrder::None;
			typeSortOrder = FileDialogSortOrder::None;
			fileNameSortOrder =
				(fileNameSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
		}
		ImGui::NextColumn();
		if (ImGui::Selectable("Size")) {
			fileNameSortOrder = FileDialogSortOrder::None;
			dateSortOrder = FileDialogSortOrder::None;
			typeSortOrder = FileDialogSortOrder::None;
			sizeSortOrder =
				(sizeSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
		}
		ImGui::NextColumn();
		if (ImGui::Selectable("Type")) {
			fileNameSortOrder = FileDialogSortOrder::None;
			dateSortOrder = FileDialogSortOrder::None;
			sizeSortOrder = FileDialogSortOrder::None;
			typeSortOrder =
				(typeSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
		}
		ImGui::NextColumn();
		if (ImGui::Selectable("Date")) {
			fileNameSortOrder = FileDialogSortOrder::None;
			sizeSortOrder = FileDialogSortOrder::None;
			typeSortOrder = FileDialogSortOrder::None;
			dateSortOrder =
				(dateSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
		}
		ImGui::NextColumn();
		ImGui::Separator();

		static auto nameSorter = [this](const io::Filesystem::DirEntry *a, const io::Filesystem::DirEntry *b) {
			if (fileNameSortOrder == FileDialogSortOrder::Down) {
				return a->name > b->name;
			}
			return a->name < b->name;
		};

		static auto sizeSorter = [this](const io::Filesystem::DirEntry *a, const io::Filesystem::DirEntry *b) {
			if (sizeSortOrder == FileDialogSortOrder::Down) {
				return a->size > b->size;
			}
			return a->size < b->size;
		};

		static auto extensionSorter = [this](const io::Filesystem::DirEntry *a, const io::Filesystem::DirEntry *b) {
			const core::String &aext = core::string::extractExtension(a->name);
			const core::String &bext = core::string::extractExtension(b->name);
			if (typeSortOrder == FileDialogSortOrder::Down) {
				return aext > bext;
			}
			return aext < bext;
		};

		static auto mtimeSorter = [this](const io::Filesystem::DirEntry *a, const io::Filesystem::DirEntry *b) {
			if (dateSortOrder == FileDialogSortOrder::Down) {
				return a->mtime > b->mtime;
			}
			return a->mtime < b->mtime;
		};

		// Sort files
		if (fileNameSortOrder != FileDialogSortOrder::None) {
			core::sort(files.begin(), files.end(), nameSorter);
		} else if (sizeSortOrder != FileDialogSortOrder::None) {
			core::sort(files.begin(), files.end(), sizeSorter);
		} else if (typeSortOrder != FileDialogSortOrder::None) {
			core::sort(files.begin(), files.end(), extensionSorter);
		} else if (dateSortOrder != FileDialogSortOrder::None) {
			core::sort(files.begin(), files.end(), mtimeSorter);
		}

		for (size_t i = 0; i < files.size(); ++i) {
			if (ImGui::Selectable(files[i]->name.c_str(), i == fileDialogFileSelectIndex,
								  ImGuiSelectableFlags_AllowDoubleClick,
								  ImVec2(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x, 0))) {
				fileDialogFileSelectIndex = i;
				fileDialogCurrentFile = files[i]->name;
				fileDialogCurrentFolder = "";
				fileDialogError[0] = '\0';
			}
			ImGui::NextColumn();
			ImGui::TextUnformatted(core::string::humanSize(files[i]->size).c_str());
			ImGui::NextColumn();
			ImGui::TextUnformatted(core::string::extractExtension(files[i]->name).c_str());
			ImGui::NextColumn();
			const core::String &lastModified = core::TimeProvider::toString(files[i]->mtime);
			ImGui::TextUnformatted(lastModified.c_str());
			ImGui::NextColumn();
		}
		ImGui::EndChild();

		core::String selectedFilePath =
			assemblePath(fileDialogCurrentPath,
						 fileDialogCurrentFolder.size() > 0 ? fileDialogCurrentFolder : fileDialogCurrentFile);
		ImGui::PushItemWidth(724);
		ImGui::InputText("", selectedFilePath.c_str(), selectedFilePath.size(), ImGuiInputTextFlags_ReadOnly);
		if (type == video::WindowedApp::OpenFileMode::Save) {
			ImGui::InputText("Filename", &fileDialogCurrentFile);
		}
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::Size(6));

		if (ImGui::Button("New folder")) {
			ImGui::OpenPopup("NewFolderPopup");
		}
		ImGui::SameLine();

		disableDeleteButton = (fileDialogCurrentFolder == "");
		if (disableDeleteButton) {
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if (ImGui::Button("Delete folder")) {
			ImGui::OpenPopup("DeleteFolderPopup");
		}
		if (disableDeleteButton) {
			ImGui::PopStyleVar();
			ImGui::PopItemFlag();
		}

		ImVec2 center(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x * 0.5f,
					  ImGui::GetWindowPos().y + ImGui::GetWindowSize().y * 0.5f);
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopup("NewFolderPopup", ImGuiWindowFlags_Modal)) {
			ImGui::Text("Enter a name for the new folder");
			ImGui::InputText("", newFolderName, sizeof(newFolderName));
			if (ImGui::Button("Create##1")) {
				if (strlen(newFolderName) <= 0) {
					SDL_strlcpy(newFolderError, "Folder name can't be empty", sizeof(newFolderError));
				} else {
					const core::String &newFilePath = assemblePath(fileDialogCurrentPath, newFolderName);
					io::filesystem()->createDir(newFilePath);
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel##1")) {
				newFolderName[0] = '\0';
				newFolderError[0] = '\0';
				ImGui::CloseCurrentPopup();
			}
			ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "%s", newFolderError);
			ImGui::EndPopup();
		}

		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopup("DeleteFolderPopup", ImGuiWindowFlags_Modal)) {
			ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "Are you sure you want to delete this folder?");
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
			ImGui::TextUnformatted(fileDialogCurrentFolder.c_str());
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
			if (ImGui::Button("Yes")) {
				const core::String &removePath = assemblePath(fileDialogCurrentPath, fileDialogCurrentFolder);
				if (!io::filesystem()->removeDir(removePath, false)) {
					Log::warn("Failed to delete directory '%s'", removePath.c_str());
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("No")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		const ImVec2 filterTextSize = ImGui::CalcTextSize(_filter.c_str());
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - filterTextSize.x - ImGui::Size(25.0f));
		if (ImGui::InputText("Filter", &_filter)) {
			applyFilter();
		}

		const char *buttonText = "Choose";
		if (type == video::WindowedApp::OpenFileMode::Open) {
			buttonText = "Open";
		} else if (type == video::WindowedApp::OpenFileMode::Save) {
			buttonText = "Save";
		}

		const ImVec2 cancelTextSize = ImGui::CalcTextSize("Cancel");
		const ImVec2 chooseTextSize = ImGui::CalcTextSize(buttonText);
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - cancelTextSize.x - chooseTextSize.x - ImGui::Size(40.0f));
		if (ImGui::Button("Cancel")) {
			fileDialogFileSelectIndex = 0;
			fileDialogFolderSelectIndex = 0;
			fileDialogCurrentFile = "";
			core_assert(bufferSize >= 1);
			buffer[0] = '\0';
			if (open != nullptr) {
				*open = false;
			}
			ImGui::End();
			return true;
		}
		ImGui::SameLine();
		if (ImGui::Button(buttonText)) {
			if (type == video::WindowedApp::OpenFileMode::Directory) {
				if (fileDialogCurrentFolder == "") {
					SDL_strlcpy(fileDialogError, "Error: You must select a folder!", sizeof(fileDialogError));
				} else {
					const core::String &fullPath = assemblePath(fileDialogCurrentPath, fileDialogCurrentFolder);
					SDL_strlcpy(buffer, fullPath.c_str(), bufferSize);
					fileDialogFileSelectIndex = 0;
					fileDialogFolderSelectIndex = 0;
					fileDialogCurrentFile = "";
					if (open != nullptr) {
						*open = false;
					}
					fileDialogError[0] = '\0';
					ImGui::End();
					return true;
				}
			} else if (type == video::WindowedApp::OpenFileMode::Open || type == video::WindowedApp::OpenFileMode::Save) {
				if (fileDialogCurrentFile == "") {
					SDL_strlcpy(fileDialogError, "Error: You must select a file!", sizeof(fileDialogError));
				} else {
					const core::String &fullPath = assemblePath(fileDialogCurrentPath, fileDialogCurrentFile);
					SDL_strlcpy(buffer, fullPath.c_str(), bufferSize);
					fileDialogFileSelectIndex = 0;
					fileDialogFolderSelectIndex = 0;
					fileDialogCurrentFile = "";
					if (open != nullptr) {
						*open = false;
					}
					fileDialogError[0] = '\0';
					ImGui::End();
					return true;
				}
			}
		}

		if (strlen(fileDialogError) > 0) {
			ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "%s", fileDialogError);
		}

		ImGui::End();
	}
	return false;
}

}
}
