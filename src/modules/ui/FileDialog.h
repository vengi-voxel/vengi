/**
 * @file
 * @ingroup UI
 * @note Used https://github.com/Limeoats/L2DFileDialog as a base
 */

#pragma once

#include "core/TimedValue.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
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

#define FILE_ALREADY_EXISTS_POPUP "###fileoverwritepopup"
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
	core::DynamicArray<const io::FilesystemEntry*> _filteredEntities;
	io::FilesystemEntry _parentDir;

	using TimedError = core::TimedValue<core::String>;
	TimedError _error;
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

	bool _scrollToSelection = false;

	io::FilesystemEntry _newFolderName;
	TimedError _newFolderError;

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
	bool popupOptions(video::FileDialogOptions &options, core::String &entityPath, video::OpenFileMode type,
								const io::FormatDescription **formatDesc);

	void filter(video::OpenFileMode type);
	/**
	 * @return @c true if a file was double clicked
	 */
	bool entitiesPanel(video::OpenFileMode type, int height);
	void showError(const TimedError &error) const;

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

	void onKeyPress(void *windowHandle, int32_t key, int16_t modifier);

	bool openDir(video::OpenFileMode type, const io::FormatDescription* formats, const core::String& filename = "");
	/**
	* @param entityPath Output buffer for the full path of the selected entity
	* @return @c true if user input was made - either an entity was selected, or the selection was cancelled.
	* @return @c false if no user input was made yet and the dialog should still run
	*/
	bool showFileDialog(video::FileDialogOptions &fileDialogOptions, core::String &entityPath, video::OpenFileMode type,
						const io::FormatDescription **formatDesc = nullptr);
};

}
