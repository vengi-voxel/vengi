/**
 * @file
 */

#include "PaletteView.h"
#include "app/Async.h"
#include "color/Color.h"
#include "core/Algorithm.h"
#include "core/Common.h"
#include "palette/Palette.h"
#include <glm/vec3.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/norm.hpp>

namespace palette {

PaletteView::PaletteView(Palette *palette) : _palette(palette) {
	sortOriginal();
}

void PaletteView::exchangeUIIndices(uint8_t palettePanelIdx1, uint8_t palettePanelIdx2) {
	if (palettePanelIdx1 == palettePanelIdx2) {
		return;
	}
	core::exchange(_uiIndices[palettePanelIdx1], _uiIndices[palettePanelIdx2]);
	_palette->markDirty();
	_palette->markSave();
}

void PaletteView::sortOriginal() {
	for (int i = 0; i < PaletteMaxColors; ++i) {
		_uiIndices[i] = i;
	}
	_palette->markDirty();
}

void PaletteView::sortHue() {
	app::sort_parallel(_uiIndices, &_uiIndices[_palette->size()], [this](uint8_t lhs, uint8_t rhs) {
		float lhhue = 0.0f;
		float lhsaturation = 0.0f;
		float lhbrightness = 0.0f;

		float rhhue = 0.0f;
		float rhsaturation = 0.0f;
		float rhbrightness = 0.0f;

		color::Color::getHSB(_palette->color(lhs), lhhue, lhsaturation, lhbrightness);
		color::Color::getHSB(_palette->color(rhs), rhhue, rhsaturation, rhbrightness);
		return lhhue < rhhue;
	});
	_palette->markDirty();
}

void PaletteView::sortSaturation() {
	app::sort_parallel(_uiIndices, &_uiIndices[_palette->size()], [this](uint8_t lhs, uint8_t rhs) {
		float lhhue = 0.0f;
		float lhsaturation = 0.0f;
		float lhbrightness = 0.0f;

		float rhhue = 0.0f;
		float rhsaturation = 0.0f;
		float rhbrightness = 0.0f;

		color::Color::getHSB(_palette->color(lhs), lhhue, lhsaturation, lhbrightness);
		color::Color::getHSB(_palette->color(rhs), rhhue, rhsaturation, rhbrightness);
		return lhsaturation < rhsaturation;
	});
	_palette->markDirty();
}

void PaletteView::sortBrightness() {
	app::sort_parallel(_uiIndices, &_uiIndices[_palette->size()], [this](uint8_t lhs, uint8_t rhs) {
		return color::Color::brightness(_palette->color(lhs)) < color::Color::brightness(_palette->color(rhs));
	});
	_palette->markDirty();
}

void PaletteView::sortCIELab() {
	app::sort_parallel(_uiIndices, &_uiIndices[_palette->size()], [this](uint8_t lhs, uint8_t rhs) {
		glm::vec3 lcielab;
		glm::vec3 rcielab;
		color::Color::getCIELab(_palette->color(lhs), lcielab.x, lcielab.y, lcielab.z);
		color::Color::getCIELab(_palette->color(rhs), rcielab.x, rcielab.y, rcielab.z);
		return glm::length2(lcielab) < glm::length2(rcielab);
	});
	_palette->markDirty();
}

} // namespace palette
