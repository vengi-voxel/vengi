/**
 * @file
 * @ingroup UI
 * @note Used https://github.com/Limeoats/L2DFileDialog as a base
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"
#include "video/WindowedApp.h"

namespace ui {
namespace imgui {

enum class FileDialogSortOrder { Up, Down, None };

class FileDialog {
private:
	char fileDialogError[500] = "";
	size_t fileDialogFileSelectIndex = 0;
	size_t fileDialogFolderSelectIndex = 0;
	core::String fileDialogCurrentPath;
	core::String fileDialogCurrentFile;
	core::String fileDialogCurrentFolder;
	core::DynamicArray<io::Filesystem::DirEntry> entities;
	core::DynamicArray<const io::Filesystem::DirEntry*> files;
	FileDialogSortOrder fileNameSortOrder = FileDialogSortOrder::None;
	FileDialogSortOrder sizeSortOrder = FileDialogSortOrder::None;
	FileDialogSortOrder dateSortOrder = FileDialogSortOrder::None;
	FileDialogSortOrder typeSortOrder = FileDialogSortOrder::None;
	core::String _filter;
	bool disableDeleteButton = false;
	char newFolderName[500] = "";
	char newFolderError[500] = "";

	void applyFilter();
	bool readDir();

public:
	/**
	 * @param filter Filter wildcard
	 */
	bool openDir(const core::String& filter);
	/**
	* @param open The visibility state of the dialog. This is set to false if the dialog should get closed.
	* This happens on user input. If the function returns @c true the boolean is set to false to no longer show
	* the dialog.
	* @param buffer Output buffer for the full path of the selected entity
	* @param bufferSize Output buffer size
	* @return @c true if user input was made - either an entity was selected, or the selection was cancelled.
	* @return @c false if no user input was made yet and the dialog should still run
	*/
	bool showFileDialog(bool *open, char *buffer, unsigned int bufferSize, video::WindowedApp::OpenFileMode type);
};

}
}
