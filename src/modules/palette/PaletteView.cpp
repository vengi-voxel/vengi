/**
 * @file
 */

#include "PaletteView.h"
#include "core/Color.h"
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
	core::sort(_uiIndices, &_uiIndices[_palette->size()], [this](uint8_t lhs, uint8_t rhs) {
		float lhhue = 0.0f;
		float lhsaturation = 0.0f;
		float lhbrightness = 0.0f;
		const glm::vec4 &lhc = core::Color::fromRGBA(_palette->color(lhs));

		float rhhue = 0.0f;
		float rhsaturation = 0.0f;
		float rhbrightness = 0.0f;
		const glm::vec4 &rhc = core::Color::fromRGBA(_palette->color(rhs));

		core::Color::getHSB(lhc, lhhue, lhsaturation, lhbrightness);
		core::Color::getHSB(rhc, rhhue, rhsaturation, rhbrightness);
		return lhhue < rhhue;
	});
	_palette->markDirty();
}

void PaletteView::sortSaturation() {
	core::sort(_uiIndices, &_uiIndices[_palette->size()], [this](uint8_t lhs, uint8_t rhs) {
		float lhhue = 0.0f;
		float lhsaturation = 0.0f;
		float lhbrightness = 0.0f;
		const glm::vec4 &lhc = core::Color::fromRGBA(_palette->color(lhs));

		float rhhue = 0.0f;
		float rhsaturation = 0.0f;
		float rhbrightness = 0.0f;
		const glm::vec4 &rhc = core::Color::fromRGBA(_palette->color(rhs));

		core::Color::getHSB(lhc, lhhue, lhsaturation, lhbrightness);
		core::Color::getHSB(rhc, rhhue, rhsaturation, rhbrightness);
		return lhsaturation < rhsaturation;
	});
	_palette->markDirty();
}

void PaletteView::sortBrightness() {
	core::sort(_uiIndices, &_uiIndices[_palette->size()], [this](uint8_t lhs, uint8_t rhs) {
		float lhhue = 0.0f;
		float lhsaturation = 0.0f;
		float lhbrightness = 0.0f;
		const glm::vec4 &lhc = core::Color::fromRGBA(_palette->color(lhs));

		float rhhue = 0.0f;
		float rhsaturation = 0.0f;
		float rhbrightness = 0.0f;
		const glm::vec4 &rhc = core::Color::fromRGBA(_palette->color(rhs));

		core::Color::getHSB(lhc, lhhue, lhsaturation, lhbrightness);
		core::Color::getHSB(rhc, rhhue, rhsaturation, rhbrightness);
		return lhbrightness < rhbrightness;
	});
	_palette->markDirty();
}

void PaletteView::sortCIELab() {
	core::sort(_uiIndices, &_uiIndices[_palette->size()], [this](uint8_t lhs, uint8_t rhs) {
		float lhL = 0.0f;
		float lha = 0.0f;
		float lhb = 0.0f;
		const glm::vec4 &lhc = core::Color::fromRGBA(_palette->color(lhs));

		float rhL = 0.0f;
		float rha = 0.0f;
		float rhb = 0.0f;
		const glm::vec4 &rhc = core::Color::fromRGBA(_palette->color(rhs));

		core::Color::getCIELab(lhc, lhL, lha, lhb);
		core::Color::getCIELab(rhc, rhL, rha, rhb);
		const glm::vec3 lcielab(lhL, lha, lhb);
		const glm::vec3 rcielab(rhL, rha, rhb);
		return glm::length2(lcielab) < glm::length2(rcielab);
	});
	_palette->markDirty();
}

} // namespace palette
