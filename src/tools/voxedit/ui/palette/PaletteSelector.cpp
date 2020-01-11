/**
 * @file
 */

#include "PaletteSelector.h"
#include "voxedit-util/SceneManager.h"
#include "core/App.h"
#include "core/io/Filesystem.h"
#include "voxel/MaterialColor.h"

namespace voxedit {

static const char *PALETTELIST = "palettes";

PaletteSelector::PaletteSelector(ui::turbobadger::Window* window) :
		Super(window) {
	core_assert_always(loadResourceFile("ui/window/voxedit-palette-selector.tb.txt"));

	_currentSelectedPalette = voxel::getDefaultPaletteName();

	std::vector<io::Filesystem::DirEntry> entities;
	io::filesystem()->list("", entities, "palette-*.png");
	if (entities.empty()) {
		Log::error("Could not find any palettes");
	}
	for (const io::Filesystem::DirEntry& file : entities) {
		if (file.type != io::Filesystem::DirEntry::Type::file) {
			continue;
		}
		const std::string& name = voxel::extractPaletteName(file.name);
		_paletteList.addItem(new tb::TBGenericStringItem(name.c_str()));
	}

	if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(PALETTELIST)) {
		select->setSource(&_paletteList);
		const int n = _paletteList.getNumItems();
		for (int i = 0; i < n; ++i) {
			if (!strcmp(_paletteList.getItemString(i), _currentSelectedPalette.c_str())) {
				select->setValue(i);
				break;
			}
		}
	}
}

PaletteSelector::~PaletteSelector() {
	if (tb::TBSelectList *select = getWidgetByType<tb::TBSelectList>(PALETTELIST)) {
		select->setSource(nullptr);
	}
}

bool PaletteSelector::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->getID() == TBIDC("ok")) {
			sceneMgr().loadPalette(_currentSelectedPalette);
			close();
			return true;
		} else if (ev.target->getID() == TBIDC("cancel")) {
			close();
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		if (ev.special_key == tb::TB_KEY_ESC) {
			close();
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_CHANGED) {
		if (ev.target->getID() == TBIDC(PALETTELIST)) {
			const tb::TBSelectList *select = (const tb::TBSelectList *)ev.target;
			const int n = select->getValue();
			const char *name = _paletteList.getItemString(n);
			_currentSelectedPalette = name;
			Log::info("%i: %s", n, name);
			return true;
		}
	}
	return Super::onEvent(ev);
}

}
