/**
 * @file
 * @ingroup UI
 * @note Used https://github.com/Limeoats/L2DFileDialog as a base
 */

#pragma once

#include "core/TimedValue.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "core/collection/RingBuffer.h"
#include "io/FilesystemEntry.h"
#include "ui/Panel.h"
#include "video/FileDialogOptions.h"
#ifdef __EMSCRIPTEN__
#include "io/system/emscripten_browser_file.h"
#endif

namespace io {
struct FormatDescription;
}

namespace ui {

using LastOpenedFiles = core::RingBuffer<core::String, 10>;

#define FILE_ALREADY_EXISTS_POPUP "###fileoverwritepopup"
#define FILE_NOT_WRITEABLE_POPUP "###filenotwriteable"
#define OPTIONS_POPUP "###optionspopup"
#define NEW_FOLDER_POPUP "###newfolderpopup"

class FileDialog : public Panel {
private:
	using Super = Panel;
	// current active path
	core::String _currentPath;
	// cached file system content of the current directory
	core::DynamicArray<io::FilesystemEntry> _entities;
	// sorted and filtered pointers to the cached file system entities
	core::Buffer<const io::FilesystemEntry*> _filteredEntities;
	io::FilesystemEntry _parentDir;

	using TimedString = core::TimedValue<core::String>;
	TimedString _error;
	size_t _entryIndex = 0;
	io::FilesystemEntry _selectedEntry;
	video::OpenFileMode _type = video::OpenFileMode::Open;
	float _filterTextWidth = 0.0f;
	int _currentFilterEntry = -1;
	io::FormatDescription *_currentFilterFormat = nullptr;
	core::String _filterAll;
	core::DynamicArray<io::FormatDescription> _filterEntries;

	core::VarPtr _showHidden;
	core::VarPtr _bookmarks;
	core::VarPtr _lastDirVar;
	core::VarPtr _lastFilterSave;
	core::VarPtr _lastFilterOpen;

	// used when e.g. changing the directory
	bool _needsSorting = false;
	bool _scrollToSelection = false;
	bool _acceptInput = false;
	TimedString _scrollToText;

	io::FilesystemEntry _newFolderName;
	TimedString _newFolderError;

	core::String _dragAndDropName;

	void setCurrentPath(video::OpenFileMode type, const core::String& path);
	void selectFilter(video::OpenFileMode type, int index);
	bool hide(const core::String &file) const;
	void resetState();
	void applyFilter(video::OpenFileMode type);
	bool readDir(video::OpenFileMode type);
	void removeBookmark(const core::String &bookmark);
	void addBookmark(const core::String &bookmark);
	bool quickAccessEntry(int index, video::OpenFileMode type, const core::String& path, float width, const char *title = nullptr, const char *icon = nullptr);
	void quickAccessPanel(video::OpenFileMode type, const core::String &bookmarks, int height);
	void currentPathPanel(video::OpenFileMode type);
	bool buttons(core::String &entityPath, video::OpenFileMode type, bool doubleClickedFile);
	void popupNewFolder();
	bool popupAlreadyExists();
	void popupNotWriteable();
	bool popupOptions(video::FileDialogOptions &options, core::String &entityPath, video::OpenFileMode type,
								const io::FormatDescription **formatDesc);

	void filter(video::OpenFileMode type);
	/**
	 * @return @c true if a file was double clicked
	 */
	bool entitiesPanel(video::OpenFileMode type, int height);
	void showError(const TimedString &error) const;

#ifdef __EMSCRIPTEN__
	static void uploadHandler(std::string const& filename, std::string const& mimetype, std::string_view buffer, void* userdata);
#endif

public:
	FileDialog(ui::IMGUIApp *app) : Super(app, "filedialog") {
	}
	virtual ~FileDialog() {
	}
	void construct();

#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *, const char *) override;
#endif

	static const char *popupTitle(video::OpenFileMode type);

	void onTextInput(void *windowHandle, const core::String &text);

	bool openDir(video::OpenFileMode type, const io::FormatDescription* formats, const core::String& filename = "");
	/**
	* @param entityPath Output buffer for the full path of the selected entity
	* @return @c true if user input was made - either an entity was selected, or the selection was cancelled.
	* @return @c false if no user input was made yet and the dialog should still run
	* @note If you want to close the dialog, set @c showFileDialog to @c false
	*/
	bool showFileDialog(video::FileDialogOptions &fileDialogOptions, core::String &entityPath, video::OpenFileMode type,
						const io::FormatDescription **formatDesc, bool &showFileDialog);
};

}
