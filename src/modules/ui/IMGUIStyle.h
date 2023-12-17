/**
 * @file
 * @ingroup UI
 */

#pragma once

namespace ImGui {

enum {
	StyleCorporateGrey = 0,
	StyleDark = 1,
	StyleLight = 2,
	StyleClassic = 3,

	MaxStyles
};

void StyleColorsCorporateGrey();
void StyleColorsNeoSequencer(int style);

}
