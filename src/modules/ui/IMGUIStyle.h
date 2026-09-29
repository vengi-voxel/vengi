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
	StyleDarkPastel = 4,
	StyleRoseQuartz = 5,
	StyleGruvboxHard = 6,
	StyleDracula = 7,

	MaxStyles
};

/**
 * @brief Get the name of a UI style
 * @param style The style index (see StyleCorporateGrey ... StyleDracula)
 * @return The name of the style
 */
const char *GetStyleName(int style);

void StyleColorsCorporateGrey();
void StyleColorsDarkPastel();
void StyleColorsRoseQuartz();
void StyleColorsGruvboxHard();
void StyleColorsDracula();
void StyleColorsNeoSequencer();
void StyleImGuizmo();
/**
 * @brief Slightly increase default Dear ImGui padding and item spacing so
 * widgets and panels have more breathing room. Call after resetting the style
 * and applying a color theme, and before ScaleAllSizes().
 */
void StyleApplySpacing();

}
