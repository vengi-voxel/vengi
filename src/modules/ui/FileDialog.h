/**
 * @file
 * @ingroup UI
 * @note Used https://github.com/Limeoats/L2DFileDialog as a base
 */

#pragma once

#include "core/TimedValue.h"
#include "core/collection/DynamicArray.h"
#include "io/FilesystemEntry.h"
#include "core/Var.h"
#include "video/FileDialogOptions.h"

namespace io {
struct FormatDescription;
}

namespace ui {

class FileDialog {
private:
	// current active path
	core::String _currentPath;
	// cached file system content of the current directory
	core::DynamicArray<io::FilesystemEntry> _entities;
	// sorted and filtered pointers to the cached file system entities
	core::DynamicArray<const io::FilesystemEntry*> _files;

	using TimedError = core::TimedValue<core::String>;
	TimedError _error;
	size_t _entryIndex = 0;
	io::FilesystemEntry _selectedEntry;

	float _filterTextWidth = 0.0f;
	int _currentFilterEntry = -1;
	io::FormatDescription *_currentFilterFormat = nullptr;
	core::String _filterAll;
	core::DynamicArray<io::FormatDescription> _filterEntries;

	core::VarPtr _showHidden;
	core::VarPtr _bookmarks;
	core::VarPtr _lastDirVar;
	core::VarPtr _lastFilter;

	io::FilesystemEntry _newFolderName;
	TimedError _newFolderError;

	void setCurrentPath(video::OpenFileMode type, const core::String& path);
	void selectFilter(video::OpenFileMode type, int index);
	bool hide(const core::String &file) const;
	void resetState();
	void applyFilter(video::OpenFileMode type);
	bool readDir(video::OpenFileMode type);
	void removeBookmark(const core::String &bookmark);
	void quickAccessEntry(video::OpenFileMode type, const core::String& path, float width, const char *title = nullptr, const char *icon = nullptr);
	void quickAccessPanel(video::OpenFileMode type, const core::String &bookmarks);
	void currentPathPanel();
	bool buttons(core::String &buffer, video::OpenFileMode type, bool doubleClickedFile);
	bool popups();
	void filter(video::OpenFileMode type);
	/**
	 * @return @c true if a file was double clicked
	 */
	bool filesPanel(video::OpenFileMode type);

public:
	void construct();

	bool openDir(video::OpenFileMode type, const io::FormatDescription* formats, const core::String& filename = "");
	/**
	* @param open The visibility state of the dialog. This is set to false if the dialog should get closed.
	* This happens on user input. If the function returns @c true the boolean is set to false to no longer show
	* the dialog.
	* @param buffer Output buffer for the full path of the selected entity
	* @return @c true if user input was made - either an entity was selected, or the selection was cancelled.
	* @return @c false if no user input was made yet and the dialog should still run
	*/
	bool showFileDialog(bool *open, video::FileDialogOptions &fileDialogOptions, core::String &buffer, video::OpenFileMode type, const io::FormatDescription **formatDesc = nullptr);
};

}
