/**
 * @file
 * @ingroup UI
 * @note Used https://github.com/Limeoats/L2DFileDialog as a base
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "io/FilesystemEntry.h"
#include "core/Var.h"
#include "video/FileDialogOptions.h"

namespace io {
struct FormatDescription;
}

namespace ui {

enum class FileDialogSortOrder { Up, Down, None };

class FileDialog {
private:
	char _error[500] = "";
	size_t _fileSelectIndex = 0;
	size_t _folderSelectIndex = 0;
	core::String _currentPath;
	core::String _currentFile;
	core::String _currentFolder;
	core::DynamicArray<io::FilesystemEntry> _entities;
	core::DynamicArray<const io::FilesystemEntry*> _files;
	FileDialogSortOrder _fileNameSortOrder = FileDialogSortOrder::None;
	FileDialogSortOrder _sizeSortOrder = FileDialogSortOrder::None;
	FileDialogSortOrder _dateSortOrder = FileDialogSortOrder::None;
	FileDialogSortOrder _typeSortOrder = FileDialogSortOrder::None;
	float _filterTextWidth = 0.0f;
	int _currentFilterEntry = -1;
	core::String _filterAll;
	core::DynamicArray<io::FormatDescription> _filterEntries;
	core::VarPtr _showHidden;
	bool _disableDeleteButton = false;
	char _newFolderName[500] = "";
	char _newFolderError[500] = "";

	void setCurrentPath(video::OpenFileMode type, const core::String& path);
	void selectFilter(int index);
	bool hide(const core::String &file) const;

	void applyFilter();
	bool readDir();
	void directoryPanel(video::OpenFileMode type);
	void removeBookmark(const core::String &bookmark);
	void bookMarkEntry(video::OpenFileMode type, const core::String& path, float width, const char *title = nullptr, const char *icon = nullptr);
	void bookmarkPanel(video::OpenFileMode type, const core::String &bookmarks);
	/**
	 * @return @c true if a file was double clicked
	 */
	bool filesPanel();

public:
	bool openDir(const io::FormatDescription* formats, const core::String& filename = "");
	/**
	* @param open The visibility state of the dialog. This is set to false if the dialog should get closed.
	* This happens on user input. If the function returns @c true the boolean is set to false to no longer show
	* the dialog.
	* @param buffer Output buffer for the full path of the selected entity
	* @param bufferSize Output buffer size
	* @return @c true if user input was made - either an entity was selected, or the selection was cancelled.
	* @return @c false if no user input was made yet and the dialog should still run
	*/
	bool showFileDialog(bool *open, video::FileDialogOptions &fileDialogOptions, char *buffer, unsigned int bufferSize, video::OpenFileMode type);
};

}
