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

bool FileDialogItemSource::Filter(int index, const char *filter) {
	const FileDialogItem* item = GetItem(index);
	if (item == nullptr) {
		return false;
	}

	const io::Filesystem::DirEntry& entry = item->entry();

	// never filter directories if we want to see directories - because we
	// always want to be able to switch to those directories
	if (entry.type == io::Filesystem::DirEntry::Type::dir && _mode != video::WindowedApp::OpenFileMode::Directory) {
		return true;
	}

	// filters might be separated by a ,
	char buf[4096];
	strncpy(buf, filter, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	char *sep = strchr(buf, ',');
	if (sep == nullptr) {
		char patternBuf[32];
		SDL_snprintf(patternBuf, sizeof(patternBuf), "*.%s", buf);
		return core::string::matches(patternBuf, item->str.CStr());
	}

	char *f = buf;
	while (*sep == ',') {
		*sep = '\0';
		char patternBuf[32];
		SDL_snprintf(patternBuf, sizeof(patternBuf), "*.%s", f);
		if (core::string::matches(patternBuf, item->str.CStr())) {
			return true;
		}
		f = ++sep;
		sep = strchr(f, ',');
		if (sep == nullptr) {
			break;
		}
	}
	char patternBuf[32];
	SDL_snprintf(patternBuf, sizeof(patternBuf), "*.%s", f);
	return core::string::matches(patternBuf, item->str.CStr());
}

tb::TBWidget *FileDialogItemSource::CreateItemWidget(int index, tb::TBSelectItemViewer *viewer) {
	return new FileDialogItemWidget(GetItem(index));
}

FileDialogWindow::FileDialogWindow(UIApp* tool, const std::function<void(const std::string&)>& callback) :
		Super(tool), _callback(callback) {
	_fs = tool->filesystem();
	loadResourceFile("ui/window/filedialog.tb.txt");
	if (tb::TBSelectList * select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
		select->SetSource(&_entityList);
		select->GetScrollContainer()->SetScrollMode(tb::SCROLL_MODE_X_AUTO_Y_AUTO);
	}
	if (tb::TBSelectDropdown * select = getWidgetByType<tb::TBSelectDropdown>(FILTERLIST)) {
		select->SetSource(&_filterList);
	}

	_filterList.SetSort(tb::TB_SORT_ASCENDING);
	_directory = _fs->absolutePath(".");
	setMode(video::WindowedApp::OpenFileMode::Open);
}

FileDialogWindow::~FileDialogWindow() {
	if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
		select->SetSource(nullptr);
	}
	if (tb::TBSelectDropdown * select = getWidgetByType<tb::TBSelectDropdown>(FILTERLIST)) {
		select->SetSource(nullptr);
	}
}

void FileDialogWindow::setMode(video::WindowedApp::OpenFileMode mode) {
	_mode = mode;
	_entityList.setMode(mode);
}

void FileDialogWindow::setFilter(const char **filter) {
	_filterList.DeleteAllItems();
	tb::TBSelectDropdown * select = getWidgetByType<tb::TBSelectDropdown>(FILTERLIST);
	if (filter == nullptr) {
		if (select != nullptr) {
			select->SetVisibility(tb::WIDGET_VISIBILITY_INVISIBLE);
		}
		return;
	}
	for (const char** f = filter; *f; ++f) {
		_filterList.AddItem(new tb::TBGenericStringItem(*f));
	}
	_filterList.AddItem(new tb::TBGenericStringItem("*"));
	if (select != nullptr && _filterList.GetNumItems() > 0) {
		select->SetValue(0);
		select->SetVisibility(tb::WIDGET_VISIBILITY_VISIBLE);
	}
}

bool FileDialogWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CHANGED && ev.target->GetID() == TBIDC(FILTERLIST)) {
		if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
			select->SetFilter(ev.target->GetText());
			return true;
		}
	}
	if (ev.type == tb::EVENT_TYPE_KEY_DOWN && ev.special_key == tb::TB_KEY_ESC) {
		tb::TBWidgetEvent click_ev(tb::EVENT_TYPE_CLICK);
		m_close_button.InvokeEvent(click_ev);
		return true;
	}
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
	} else if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (id == TBIDC("ok")) {
			if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
				const int index = select->GetValue();
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
	}

	return Super::OnEvent(ev);
}

void FileDialogWindow::OnAdded() {
	Log::info("Load entries");
	Super::OnAdded();
}

void FileDialogWindow::changeDir(const std::string& dir) {
	if (!dir.empty()) {
		_directory = _fs->absolutePath(_directory + "/" + dir);
	}
	_entityList.DeleteAllItems();
	_entityList.AddItem(new FileDialogItem(io::Filesystem::DirEntry{"..", io::Filesystem::DirEntry::Type::dir}));

	std::vector<io::Filesystem::DirEntry> entities;
	getApp()->filesystem()->list(_directory, entities);

	Log::debug("Looking in %s and found %i entries", _directory.c_str(), (int)entities.size());
	for (const io::Filesystem::DirEntry& e : entities) {
		_entityList.AddItem(new FileDialogItem(e));
	}
}

}
}
