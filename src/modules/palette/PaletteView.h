/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace palette {

static const int PaletteMaxColors = 256;

using PaletteIndicesArray = uint8_t[PaletteMaxColors];

class Palette;

/**
 * @brief UI related stuff
 */
class PaletteView {
	friend class Palette;

private:
	PaletteIndicesArray _uiIndices;
	Palette *_palette;

public:
	PaletteView(Palette *palette);
	/**
	 * In case the palette indices are changed, this gives you access to the real color index
	 */
	uint8_t uiIndex(uint8_t palettePanelIdx) const;
	const PaletteIndicesArray &uiIndices() const;
	PaletteIndicesArray &uiIndices();

	// Only sort for the ui - does not change any colors in the color array of the palette

	void sortOriginal();
	void sortHue();
	void sortSaturation();
	void sortBrightness();
	void sortCIELab();

	/**
	 * @note changes the color slots for the ui
	 */
	void exchangeUIIndices(uint8_t idx1, uint8_t idx2);
};

inline const PaletteIndicesArray &PaletteView::uiIndices() const {
	return _uiIndices;
}

inline PaletteIndicesArray &PaletteView::uiIndices() {
	return _uiIndices;
}

inline uint8_t PaletteView::uiIndex(uint8_t palettePanelIdx) const {
	return _uiIndices[palettePanelIdx];
}

} // namespace palette
