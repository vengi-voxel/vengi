/**
 * @file
 */

#include "ViewMode.h"
#include "app/I18N.h"

namespace voxedit {

const char *getViewModeString(ViewMode viewMode) {
	switch (viewMode) {
	case ViewMode::Simple:
		return _("Simple");
	case ViewMode::All:
		return _("All");
	case ViewMode::CommandAndConquer:
		return _("Command & Conquer");
	case ViewMode::Max:
	case ViewMode::Default:
		break;
	}
	return _("Default");
}

} // namespace voxedit
