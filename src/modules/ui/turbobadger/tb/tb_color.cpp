/**
 * @file
 */

#include "tb_color.h"
#include <stdio.h>

namespace tb {

void TBColor::setFromString(const char *str, int len) {
	int r, g, b, a;
	if (len == 9 && sscanf(str, "#%2x%2x%2x%2x", &r, &g, &b, &a) == 4) // rrggbbaa
		set(TBColor(r, g, b, a));
	else if (len == 7 && sscanf(str, "#%2x%2x%2x", &r, &g, &b) == 3) // rrggbb
		set(TBColor(r, g, b));
	else if (len == 5 && sscanf(str, "#%1x%1x%1x%1x", &r, &g, &b, &a) == 4) // rgba
		set(TBColor(r + (r << 4), g + (g << 4), b + (b << 4), a + (a << 4)));
	else if (len == 4 && sscanf(str, "#%1x%1x%1x", &r, &g, &b) == 3) // rgb
		set(TBColor(r + (r << 4), g + (g << 4), b + (b << 4)));
	else
		set(TBColor());
}

} // namespace tb
