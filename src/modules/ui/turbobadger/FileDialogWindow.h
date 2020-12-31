/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "ui/turbobadger/ui_widgets.h"
#include "video/WindowedApp.h"
#include "io/Filesystem.h"

namespace ui {
namespace turbobadger {

/**
 * @brief Adds extra info to a string item.
 */
class FileDialogItem: public tb::TBGenericStringItem {
private:
	const io::Filesystem::DirEntry _entry;
public:
	FileDialogItem(const io::Filesystem::DirEntry& entry) :
			tb::TBGenericStringItem(entry.name.c_str()), _entry(entry) {
	}

	inline const io::Filesystem::DirEntry& entry() const { return _entry; }
};

/**
 * @brief FileDialogItemWidget is the widget representing a FileDialogItem.
 * On changes to the item, it calls InvokeItemChanged on the source, so that all
 * viewers of the source are updated to reflect the change.
 */
class FileDialogItemWidget : public tb::TBLayout {
public:
	FileDialogItemWidget(FileDialogItem *item);
};

/**
 * @brief FileDialogItemSource provides items of type FileDialogItem and makes sure
 * the viewer is populated with the customized widget for each item.
 */
class FileDialogItemSource: public tb::TBSelectItemSourceList<FileDialogItem> {
private:
	video::WindowedApp::OpenFileMode _mode;
	bool _showHidden = false;
	bool filterHidden(const io::Filesystem::DirEntry& entry) const;
public:
	bool filter(int index, const char *filter) override;
	tb::TBWidget *createItemWidget(int index, tb::TBSelectItemViewer *viewer) override;

	inline void setMode(video::WindowedApp::OpenFileMode mode) { _mode = mode; }
	inline void setShowHidden(bool showHidden) { _showHidden = showHidden; }
};

class FileDialogWindow: public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;
	core::String _directory;
	video::WindowedApp::OpenFileMode _mode;
	FileDialogItemSource _entityList;
	tb::TBGenericStringItemSource _dirList;
	tb::TBGenericStringItemSource _filterList;
	std::function<void(const core::String&)> _callback;
	io::FilesystemPtr _fs;
	core::VarPtr _lastDirectory;
public:
	FileDialogWindow(UIApp* app, const std::function<void(const core::String&)>& callback, const core::VarPtr& lastDirectory);
	~FileDialogWindow();
	void changeDir(const core::String& dir = "");
	void init();
	void addShortcut(const core::String& dir);

	void setFilter(const char **filter);
	void setMode(video::WindowedApp::OpenFileMode mode, const char *inputText = nullptr);

	bool onEvent(const tb::TBWidgetEvent &ev) override;
};

}
}
