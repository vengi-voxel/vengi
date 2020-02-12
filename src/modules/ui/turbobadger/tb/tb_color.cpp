/**
 * @file
 */

#include "tb_color.h"
#include <stdio.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace tb {

TBColor TBColor::fromVec4(const glm::vec4& c) {
	return TBColor((int)(c.r * 255.0f), (int)(c.g * 255.0f), (int)(c.b * 255.0f), (int)(c.a * 255.0f));
}

TBColor TBColor::fromVec3(const glm::vec3& c) {
	return TBColor((int)(c.r * 255.0f), (int)(c.g * 255.0f), (int)(c.b * 255.0f));
}

void TBColor::setFromString(const char *str, int len) {
	int r;
	int g;
	int b;
	int a;
	if (len == 9 && sscanf(str, "#%2x%2x%2x%2x", &r, &g, &b, &a) == 4) { // rrggbbaa
		set(TBColor(r, g, b, a));
	} else if (len == 7 && sscanf(str, "#%2x%2x%2x", &r, &g, &b) == 3) { // rrggbb
		set(TBColor(r, g, b));
	} else if (len == 5 && sscanf(str, "#%1x%1x%1x%1x", &r, &g, &b, &a) == 4) { // rgba
		set(TBColor(r + (r << 4), g + (g << 4), b + (b << 4), a + (a << 4)));
	} else if (len == 4 && sscanf(str, "#%1x%1x%1x", &r, &g, &b) == 3) { // rgb
		set(TBColor(r + (r << 4), g + (g << 4), b + (b << 4)));
	} else {
		set(TBColor());
	}
}

} // namespace tb
