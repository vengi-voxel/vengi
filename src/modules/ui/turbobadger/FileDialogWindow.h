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
public:
	bool Filter(int index, const char *filter) override {
		return TBSelectItemSource::Filter(index, filter);
	}
	tb::TBWidget *CreateItemWidget(int index, tb::TBSelectItemViewer *viewer) override {
		return new FileDialogItemWidget(GetItem(index));
	}
};

class FileDialogWindow: public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;
	std::string _filter;
	std::string _directory;
	video::WindowedApp::OpenFileMode _mode = video::WindowedApp::OpenFileMode::Open;
	FileDialogItemSource _entityList;
	tb::TBGenericStringItemSource _filterList;
	std::function<void(const std::string&)> _callback;
	io::FilesystemPtr _fs;
public:
	FileDialogWindow(UIApp* app, const std::function<void(const std::string&)>& callback);
	~FileDialogWindow();
	void changeDir(const std::string& dir);

	void setFilter(const char **filter);
	void setMode(video::WindowedApp::OpenFileMode mode);

	void OnAdded() override;
	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

inline void FileDialogWindow::setMode(video::WindowedApp::OpenFileMode mode) {
	_mode = mode;
}

}
}
