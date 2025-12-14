/**
 * @file
 */

#include "ViewMode.h"
#include "app/I18N.h"
#include "core/ArrayLength.h"

namespace voxedit {

static const uint64_t ALL_FLAGS = ((uint64_t)-1) & ~(VIEWMODE_FLAG_PALFORMAT6BIT | VIEWMODE_FLAG_NOSPLIT);
static const uint64_t DEFAULT_FLAGS = ALL_FLAGS & ~VIEWMODE_FLAG_NORMALPALETTE;
static const uint64_t SIMLPE_FLAGS =
	DEFAULT_FLAGS &
	~(VIEWMODE_FLAG_ALL_VIEWPORTS | VIEWMODE_FLAG_MEMENTOPANEL | VIEWMODE_FLAG_CAMERAPANEL |
	  VIEWMODE_FLAG_LSYSTEMPANEL | VIEWMODE_FLAG_SCRIPTPANEL | VIEWMODE_FLAG_NETWORKPANEL);
static const uint64_t COMMANDANDCONQUER_FLAGS =
	DEFAULT_FLAGS | VIEWMODE_FLAG_NORMALPALETTE | VIEWMODE_FLAG_PALFORMAT6BIT;
static const uint64_t MINECRAFTSKIN_FLAGS =
	SIMLPE_FLAGS & ~(VIEWMODE_FLAG_ASSETPANEL | VIEWMODE_FLAG_ANIMATIONS | VIEWMODE_FLAG_RENDERPANEL);
static const uint64_t ACEOFSPADES_FLAGS = (DEFAULT_FLAGS & ~(VIEWMODE_FLAG_ANIMATIONS)) | VIEWMODE_FLAG_NOSPLIT;

static const uint64_t s_viewModeFlags[] = {
	DEFAULT_FLAGS,			 // Default
	SIMLPE_FLAGS,			 // Simple
	ALL_FLAGS,				 // All
	COMMANDANDCONQUER_FLAGS, // CommandAndConquer
	MINECRAFTSKIN_FLAGS,	 // MinecraftSkin
	ACEOFSPADES_FLAGS		 // AceOfSpades
};
static_assert(lengthof(s_viewModeFlags) == (int)ViewMode::Max, "Viewmode flags don't match existing viewmodes");

uint64_t viewModeFlags(ViewMode viewMode) {
	if (viewMode == ViewMode::Max) {
		return 0u;
	}
	return s_viewModeFlags[(int)viewMode];
}

const char *getViewModeString(ViewMode viewMode) {
	switch (viewMode) {
	case ViewMode::Simple:
		return _("Simple");
	case ViewMode::All:
		return _("All");
	case ViewMode::CommandAndConquer:
		return _("Command & Conquer");
	case ViewMode::MinecraftSkin:
		return _("Minecraft Skin");
	case ViewMode::AceOfSpades:
		return _("Ace Of Spades");
	case ViewMode::Max:
	case ViewMode::Default:
		break;
	}
	return _("Default");
}

} // namespace voxedit
