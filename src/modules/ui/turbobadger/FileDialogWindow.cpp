/**
 * @file
 */

#include "FileDialogWindow.h"

namespace ui {
namespace turbobadger {

const char *FILELIST = "files";
const char *FILTERLIST = "filter";

FileDialogItemWidget::FileDialogItemWidget(FileDialogItem *item) : tb::TBLayout() {
	SetSkinBg(TBIDC("TBSelectItem"));
	SetLayoutDistribution(tb::LAYOUT_DISTRIBUTION_GRAVITY);
	SetLayoutDistributionPosition(tb::LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
	SetPaintOverflowFadeout(false);

	if (item->entry().type == io::Filesystem::DirEntry::Type::dir) {
		tb::g_widgets_reader->LoadFile(GetContentRoot(), "ui/window/filedialog_dir.tb.txt");
	} else {
		tb::g_widgets_reader->LoadFile(GetContentRoot(), "ui/window/filedialog_file.tb.txt");
	}
	if (tb::TBTextField *name = GetWidgetByIDAndType<tb::TBTextField>(TBIDC("name"))) {
		name->SetText(item->entry().name.c_str());
	}
}

FileDialogWindow::FileDialogWindow(UIApp* tool, const std::function<void(const std::string&)>& callback) :
		Super(tool), _callback(callback) {
	_fs = tool->filesystem();
	loadResourceFile("ui/window/filedialog.tb.txt");
	if (tb::TBSelectList * select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
		select->SetSource(&_entityList);
		select->GetScrollContainer()->SetScrollMode(tb::SCROLL_MODE_X_AUTO_Y_AUTO);
	}
	if (tb::TBSelectList * select = getWidgetByType<tb::TBSelectList>(FILTERLIST)) {
		select->SetSource(&_filterList);
	}

	_directory = _fs->absolutePath(".");
	changeDir("");
}

FileDialogWindow::~FileDialogWindow() {
	if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
		select->SetSource(nullptr);
	}
}

void FileDialogWindow::setFilter(const char **filter) {
	_filter.clear();
	_filterList.DeleteAllItems();
	for (const char* f = *filter; *filter; ++filter) {
		addStringItem(_filterList, f, nullptr, false);
		_filter.append(f);
		if (*(filter + 1)) {
			_filter.append(";");
		}
		_filterList.AddItem(new tb::TBGenericStringItem(f));
	}
}

bool FileDialogWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	const tb::TBID& id = ev.target->GetID();
	if (ev.type == tb::EVENT_TYPE_POINTER_DOWN && ev.count >= 2) {
		if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
			const int index = select->GetValue();
			if (index >= 0 && index < _entityList.GetNumItems()) {
				const FileDialogItem* item = _entityList.GetItem(index);
				const auto& dirEntry = item->entry();
				if (_mode != video::WindowedApp::OpenFileMode::Directory
				 && dirEntry.type == io::Filesystem::DirEntry::Type::dir) {
					changeDir(dirEntry.name);
					return true;
				}
				_callback(_directory + "/" + dirEntry.name);
				tb::TBWidgetEvent click_ev(tb::EVENT_TYPE_CLICK);
				m_close_button.InvokeEvent(click_ev);
				return true;
			}
		}
	}
	if (ev.type == tb::EVENT_TYPE_CLICK && id == TBIDC("ok")) {
		if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
			const int index =select->GetValue();
			if (index >= 0 && index < _entityList.GetNumItems()) {
				const FileDialogItem* item = _entityList.GetItem(index);
				const auto& dirEntry = item->entry();
				_callback(_directory + "/" + dirEntry.name);
			}
		}
		tb::TBWidgetEvent click_ev(tb::EVENT_TYPE_CLICK);
		m_close_button.InvokeEvent(click_ev);
		return true;
	} else if (id == TBIDC("cancel")) {
		tb::TBWidgetEvent click_ev(tb::EVENT_TYPE_CLICK);
		m_close_button.InvokeEvent(click_ev);
		return true;
	}

	return Super::OnEvent(ev);
}

void FileDialogWindow::OnAdded() {
	Log::info("Load entries");
	Super::OnAdded();
}

void FileDialogWindow::changeDir(const std::string& dir) {
	_directory = _fs->absolutePath(_directory + "/" + dir);
	_entityList.DeleteAllItems();
	if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
		std::vector<io::Filesystem::DirEntry> entities;
		entities.push_back(io::Filesystem::DirEntry{"..", io::Filesystem::DirEntry::Type::dir});
		getApp()->filesystem()->list(_directory, entities, _filter);
		Log::debug("Looking in %s and found %i entries (filter: '%s')", _directory.c_str(), (int)entities.size(), _filter.c_str());
		for (const io::Filesystem::DirEntry& e : entities) {
			if (_mode == video::WindowedApp::OpenFileMode::Directory && e.type != io::Filesystem::DirEntry::Type::dir) {
				continue;
			}
			_entityList.AddItem(new FileDialogItem(e));
		}
	}
}

}
}
