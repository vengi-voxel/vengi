/**
 * @file
 */

#include "LUAIconMapping.h"
#include "core/ArrayLength.h"
#include "ui/IconsLucide.h"

#include <SDL_stdinc.h>

namespace voxedit {

struct LUAIconMapping {
	const char *name;
	const char *icon;
};

static const LUAIconMapping luaIconMappings[] = {
	{"blend", ICON_LC_BLEND},
	{"box", ICON_LC_BOX},
	{"boxes", ICON_LC_BOXES},
	{"brush", ICON_LC_BRUSH},
	{"circle", ICON_LC_CIRCLE},
	{"circledashed", ICON_LC_CIRCLE_DASHED},
	{"cloud", ICON_LC_CLOUD},
	{"columns3", ICON_LC_COLUMNS_3},
	{"diamond", ICON_LC_DIAMOND},
	{"eraser", ICON_LC_ERASER},
	{"expand", ICON_LC_EXPAND},
	{"flame", ICON_LC_FLAME},
	{"footprints", ICON_LC_FOOTPRINTS},
	{"grid2x2", ICON_LC_GRID_2X2},
	{"grid3x3", ICON_LC_GRID_3X3},
	{"group", ICON_LC_GROUP},
	{"hammer", ICON_LC_HAMMER},
	{"hexagon", ICON_LC_HEXAGON},
	{"image", ICON_LC_IMAGE},
	{"landplot", ICON_LC_LAND_PLOT},
	{"lasso", ICON_LC_LASSO},
	{"layers", ICON_LC_LAYERS},
	{"mountain", ICON_LC_MOUNTAIN},
	{"move", ICON_LC_MOVE},
	{"paintbrush", ICON_LC_PAINTBRUSH},
	{"palette", ICON_LC_PALETTE},
	{"penline", ICON_LC_PEN_LINE},
	{"pencil", ICON_LC_PENCIL},
	{"pipette", ICON_LC_PIPETTE},
	{"ruler", ICON_LC_RULER},
	{"scan", ICON_LC_SCAN},
	{"scroll", ICON_LC_SCROLL},
	{"snowflake", ICON_LC_SNOWFLAKE},
	{"sparkles", ICON_LC_SPARKLES},
	{"spray", ICON_LC_SPRAY_CAN},
	{"square", ICON_LC_SQUARE},
	{"squaredashed", ICON_LC_SQUARE_DASHED},
	{"stamp", ICON_LC_STAMP},
	{"star", ICON_LC_STAR},
	{"sun", ICON_LC_SUN},
	{"swords", ICON_LC_SWORDS},
	{"target", ICON_LC_TARGET},
	{"torus", ICON_LC_TORUS},
	{"trees", ICON_LC_TREES},
	{"triangle", ICON_LC_TRIANGLE},
	{"wand", ICON_LC_WAND},
	{"waves", ICON_LC_WAVES},
	{"zap", ICON_LC_ZAP},
};

const char *luaIconString(const core::String &iconName, const char *defaultIcon) {
	if (iconName.empty()) {
		return defaultIcon;
	}
	for (int i = 0; i < (int)lengthof(luaIconMappings); ++i) {
		if (SDL_strcasecmp(iconName.c_str(), luaIconMappings[i].name) == 0) {
			return luaIconMappings[i].icon;
		}
	}
	return defaultIcon;
}

} // namespace voxedit
