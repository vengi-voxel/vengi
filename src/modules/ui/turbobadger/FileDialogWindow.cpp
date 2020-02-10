/**
 * @file
 */

#include "FileDialogWindow.h"
#include "ui/turbobadger/UIApp.h"
#include <SDL_stdinc.h>

namespace ui {
namespace turbobadger {

static const char *FILELIST = "files";
static const char *DIRLIST = "dirs";
static const char *FILTERLIST = "filter";
static const char *INPUT = "input";

bool FileDialogItemSource::execFileItemFilter(const char* str, const char* filter) {
	// filters might be separated by a ,
	char buf[4096];
	SDL_strlcpy(buf, filter, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';

	char *sep = SDL_strstr(buf, ",");
	if (sep == nullptr) {
		if (!SDL_strcmp(buf, "*") || !SDL_strncmp(buf, "*.", 2)) {
			return core::string::matches(buf, str);
		}
		char patternBuf[32];
		SDL_snprintf(patternBuf, sizeof(patternBuf), "*.%s", buf);
		return core::string::matches(patternBuf, str);
	}

	char *f = buf;
	while (*sep == ',') {
		*sep = '\0';
		char patternBuf[32];
		SDL_snprintf(patternBuf, sizeof(patternBuf), "*.%s", f);
		if (core::string::matches(patternBuf, str)) {
			return true;
		}
		f = ++sep;
		sep = SDL_strchr(f, ',');
		if (sep == nullptr) {
			break;
		}
	}
	char patternBuf[32];
	SDL_snprintf(patternBuf, sizeof(patternBuf), "*.%s", f);
	return core::string::matches(patternBuf, str);
}

FileDialogItemWidget::FileDialogItemWidget(FileDialogItem *item) : tb::TBLayout() {
	setSkinBg(TBIDC("TBSelectItem"));
	setLayoutDistribution(tb::LAYOUT_DISTRIBUTION_GRAVITY);
	setLayoutDistributionPosition(tb::LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
	setPaintOverflowFadeout(false);

	if (item->entry().type == io::Filesystem::DirEntry::Type::dir) {
		tb::g_widgets_reader->loadFile(getContentRoot(), "ui/window/filedialog_dir.tb.txt");
	} else {
		tb::g_widgets_reader->loadFile(getContentRoot(), "ui/window/filedialog_file.tb.txt");
	}
	if (tb::TBTextField *name = getWidgetByIDAndType<tb::TBTextField>(TBIDC("name"))) {
		name->setText(item->entry().name.c_str());
	}
}

bool FileDialogItemSource::filterHidden(const io::Filesystem::DirEntry& entry) const {
	if (_showHidden) {
		return false;
	}
	if (entry.name == "..") {
		return false;
	}
	return entry.name[0] == '.';
}

bool FileDialogItemSource::filter(int index, const char *filter) {
	const FileDialogItem* item = getItem(index);
	if (item == nullptr) {
		return false;
	}

	const io::Filesystem::DirEntry& entry = item->entry();

	if (filterHidden(entry)) {
		return false;
	}

	// never filter directories if we want to see directories - because we
	// always want to be able to switch to those directories
	if (entry.type == io::Filesystem::DirEntry::Type::dir && _mode != video::WindowedApp::OpenFileMode::Directory) {
		return true;
	}

	return execFileItemFilter(item->str, filter);
}

tb::TBWidget *FileDialogItemSource::createItemWidget(int index, tb::TBSelectItemViewer *viewer) {
	return new FileDialogItemWidget(getItem(index));
}

FileDialogWindow::FileDialogWindow(UIApp* tool, const std::function<void(const core::String&)>& callback, const core::VarPtr& lastDirectory) :
		Super(tool), _callback(callback), _lastDirectory(lastDirectory) {
	_fs = tool->filesystem();
	loadResourceFile("ui/window/filedialog.tb.txt");
	if (tb::TBSelectList * select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
		select->setSource(&_entityList);
		select->getScrollContainer()->setScrollMode(tb::SCROLL_MODE_X_AUTO_Y_AUTO);
	}
	if (tb::TBSelectList * select = getWidgetByType<tb::TBSelectList>(DIRLIST)) {
		select->setSource(&_dirList);
		const io::Paths& paths = io::filesystem()->paths();
		for (const auto& p : paths) {
			_dirList.addItem(new tb::TBGenericStringItem(p.c_str()));
		}
	}
	if (tb::TBSelectDropdown * select = getWidgetByType<tb::TBSelectDropdown>(FILTERLIST)) {
		select->setSource(&_filterList);
	}

	_filterList.setSort(tb::TB_SORT_ASCENDING);
	_directory = _fs->absolutePath(".");
	setMode(video::WindowedApp::OpenFileMode::Open);
}

FileDialogWindow::~FileDialogWindow() {
	if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
		select->setSource(nullptr);
	}
	if (tb::TBSelectDropdown * select = getWidgetByType<tb::TBSelectDropdown>(FILTERLIST)) {
		select->setSource(nullptr);
	}
	if (tb::TBSelectList * select = getWidgetByType<tb::TBSelectList>(DIRLIST)) {
		select->setSource(nullptr);
	}
}

void FileDialogWindow::addShortcut(const core::String& dir) {
	_dirList.addItem(new tb::TBGenericStringItem(dir.c_str()));
}

void FileDialogWindow::setMode(video::WindowedApp::OpenFileMode mode, const char *inputText) {
	_mode = mode;
	_entityList.setMode(mode);
	if (tb::TBEditField * input = getWidgetByType<tb::TBEditField>(INPUT)) {
		if (_mode == video::WindowedApp::OpenFileMode::Save
				|| _mode == video::WindowedApp::OpenFileMode::Open) {
			input->setVisibility(tb::WIDGET_VISIBILITY_VISIBLE);
			input->setFocus(tb::WIDGET_FOCUS_REASON_UNKNOWN);
			if (inputText != nullptr) {
				input->setText(inputText);
			}
			if (_mode == video::WindowedApp::OpenFileMode::Save) {
				input->setPlaceholderText(tr("Enter filename for saving"));
				if (tb::TBButton * ok = getWidgetByType<tb::TBButton>("ok")) {
					ok->setState(tb::WIDGET_STATE_DISABLED, input->getText().isEmpty());
				}
			} else {
				input->setPlaceholderText(tr("Enter filename for loading"));
				if (tb::TBButton * ok = getWidgetByType<tb::TBButton>("ok")) {
					ok->setState(tb::WIDGET_STATE_DISABLED, false);
				}
			}
		} else {
			input->setVisibility(tb::WIDGET_VISIBILITY_GONE);
		}
	}
}

void FileDialogWindow::setFilter(const char **filter) {
	_filterList.deleteAllItems();
	tb::TBSelectDropdown * select = getWidgetByType<tb::TBSelectDropdown>(FILTERLIST);
	if (filter == nullptr) {
		if (select != nullptr) {
			select->setVisibility(tb::WIDGET_VISIBILITY_INVISIBLE);
		}
		return;
	}
	for (const char** f = filter; *f; ++f) {
		_filterList.addItem(new tb::TBGenericStringItem(*f));
	}
	_filterList.addItem(new tb::TBGenericStringItem("*"));
	if (select != nullptr && _filterList.getNumItems() > 0) {
		select->setValue(0);
		select->setVisibility(tb::WIDGET_VISIBILITY_VISIBLE);
	}
}

bool FileDialogWindow::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CHANGED) {
		if (ev.target->getID() == TBIDC(FILTERLIST)) {
			if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
				select->setFilter(ev.target->getText());
				return true;
			}
		} else if (ev.target->getID() == TBIDC(DIRLIST)) {
			if (tb::TBGenericStringItem* item = _dirList.getItem(ev.target->getValue())) {
				changeDir(item->str.c_str());
				if (tb::TBEditField * input = getWidgetByType<tb::TBEditField>(INPUT)) {
					input->setText(_directory.c_str());
				}
			}
			return true;
		} else if (ev.target->getID() == TBIDC(INPUT)) {
			const tb::TBStr& str = ev.target->getText();
			if (tb::TBButton * ok = getWidgetByType<tb::TBButton>("ok")) {
				bool disabled;
				if (str.isEmpty()) {
					disabled = true;
				} else {
					disabled = true;
					if (_filterList.getNumItems() == 1) {
						disabled = false;
					} else {
						for (int i = 0; i < _filterList.getNumItems(); i++) {
							const char *filter = _filterList.getItemString(i);
							if (!SDL_strcmp(filter, "*")) {
								continue;
							}
							if (FileDialogItemSource::execFileItemFilter(str.c_str(), filter)) {
								disabled = false;
								break;
							}
						}
					}
				}
				ok->setState(tb::WIDGET_STATE_DISABLED, disabled);
			}
			// if entered manually, we want to change the directory.
			if (io::Filesystem::isReadableDir(str.c_str())) {
				changeDir(str.c_str());
			}
		}
	}
	if (ev.type == tb::EVENT_TYPE_KEY_DOWN && ev.special_key == tb::TB_KEY_ESC) {
		tb::TBWidgetEvent click_ev(tb::EVENT_TYPE_CLICK);
		m_close_button.invokeEvent(click_ev);
		return true;
	}
	const tb::TBID& id = ev.target->getID();
	if (ev.type == tb::EVENT_TYPE_POINTER_DOWN && ev.count >= 2) {
		if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
			const int index = select->getValue();
			if (index >= 0 && index < _entityList.getNumItems()) {
				const FileDialogItem* item = _entityList.getItem(index);
				const auto& dirEntry = item->entry();
				if (_mode != video::WindowedApp::OpenFileMode::Directory
				 && dirEntry.type == io::Filesystem::DirEntry::Type::dir) {
					changeDir(dirEntry.name);
					if (tb::TBEditField * input = getWidgetByType<tb::TBEditField>(INPUT)) {
						input->setText(_directory.c_str());
					}
					return true;
				}
				if (_mode == video::WindowedApp::OpenFileMode::Save) {
					if (tb::TBEditField * input = getWidgetByType<tb::TBEditField>(INPUT)) {
						input->setText(dirEntry.name.c_str());
					}
				} else {
					const core::String& filename = dirEntry.name;
					if (io::Filesystem::isRelativePath(filename)) {
						_callback(_directory + "/" + filename);
					} else {
						_callback(filename);
					}
					tb::TBWidgetEvent click_ev(tb::EVENT_TYPE_CLICK);
					m_close_button.invokeEvent(click_ev);
				}
				return true;
			}
		}
	} else if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (id == TBIDC("ok")) {
			if (_mode == video::WindowedApp::OpenFileMode::Save) {
				if (tb::TBEditField * input = getWidgetByType<tb::TBEditField>(INPUT)) {
					const tb::TBStr& filename = input->getText();
					const core::String& sfilename = core::String(filename.c_str());
					if (io::Filesystem::isRelativePath(sfilename)) {
						_callback(_directory + "/" + sfilename);
					} else {
						_callback(sfilename);
					}
				} else {
					Log::error("Failed to get input node");
				}
			} else if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(FILELIST)) {
				const int index = select->getValue();
				if (index >= 0 && index < _entityList.getNumItems()) {
					const FileDialogItem* item = _entityList.getItem(index);
					const auto& dirEntry = item->entry();
					const core::String& filename = dirEntry.name;
					if (io::Filesystem::isRelativePath(filename)) {
						_callback(_directory + "/" + filename);
					} else {
						_callback(filename);
					}
				}
			}
			tb::TBWidgetEvent click_ev(tb::EVENT_TYPE_CLICK);
			m_close_button.invokeEvent(click_ev);
			return true;
		} else if (id == TBIDC("cancel")) {
			tb::TBWidgetEvent click_ev(tb::EVENT_TYPE_CLICK);
			m_close_button.invokeEvent(click_ev);
			return true;
		}
	}

	return Super::onEvent(ev);
}

void FileDialogWindow::init() {
	if (tb::TBEditField * input = getWidgetByType<tb::TBEditField>(INPUT)) {
		input->setText(_directory.c_str());
	}
}

void FileDialogWindow::changeDir(const core::String& dir) {
	if (!dir.empty()) {
		if (io::Filesystem::isRelativePath(dir)) {
			_directory = _fs->absolutePath(_directory + "/" + dir);
		} else {
			_directory = dir;
		}
		if (!io::Filesystem::isReadableDir(_directory)) {
			_directory = _fs->absolutePath(".");
		}
	}
	_lastDirectory->setVal(_directory);

	_entityList.deleteAllItems();
	_entityList.addItem(new FileDialogItem(io::Filesystem::DirEntry{"..", io::Filesystem::DirEntry::Type::dir, (uint64_t)0}));

	std::vector<io::Filesystem::DirEntry> entities;
	getApp()->filesystem()->list(_directory, entities);

	Log::debug("Looking in %s and found %i entries", _directory.c_str(), (int)entities.size());
	for (const io::Filesystem::DirEntry& e : entities) {
		_entityList.addItem(new FileDialogItem(e));
	}
}

}
}
