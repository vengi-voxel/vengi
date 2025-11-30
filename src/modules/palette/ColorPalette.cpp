/**
 * @file
 */

#include "ColorPalette.h"
#include "color/Color.h"
#include "core/Log.h"
#include "image/Image.h"
#include "palette/RGBABuffer.h"

namespace palette {

const ColorPaletteEntry *ColorPalette::entry(size_t index) const {
	if (index >= _entries.size()) {
		return nullptr;
	}
	return &_entries[index];
}

void ColorPalette::setSize(size_t size) {
	_entries.resize(size);
}

void ColorPalette::reserve(size_t size) {
	_entries.reserve(size);
}

void ColorPalette::setColor(size_t index, const color::RGBA &color) {
	if (index >= _entries.size()) {
		reserve(index + 256);
		setSize(index + 1);
	}
	_entries[index].color = color;
	markDirty();
}

void ColorPalette::setColorName(size_t index, const core::String &name) {
	if (index >= _entries.size()) {
		return;
	}
	_entries[index].name = name;
	markDirty();
}

void ColorPalette::setMaterial(size_t index, const palette::Material &material) {
	if (index >= _entries.size()) {
		return;
	}
	_entries[index].material = material;
	markDirty();
}

void ColorPalette::setName(const core::String &name) {
	_name = name;
	markDirty();
}

void ColorPalette::optimize() {
	RGBABuffer uniqueColors;
	core::DynamicArray<ColorPaletteEntry> optimizedEntries;
	optimizedEntries.reserve(_entries.size());
	for (const auto &entry : _entries) {
		if (entry.color.a == 0) {
			continue;
		}
		if (!uniqueColors.has(entry.color)) {
			uniqueColors.insert(entry.color);
			optimizedEntries.push_back(entry);
		}
	}
	_entries = optimizedEntries;
	markDirty();
}

int ColorPalette::colorCount() const {
	return (int)_entries.size();
}

const core::String &ColorPalette::name() const {
	return _name;
}

size_t ColorPalette::size() const {
	return _entries.size();
}

const core::String &ColorPalette::colorName(size_t index) const {
	const ColorPaletteEntry *e = entry(index);
	if (e == nullptr) {
		return _empty.name;
	}
	return e->name;
}

bool ColorPalette::load(const image::ImagePtr &img) {
	if (img->components() != 4) {
		Log::warn("Palette image has invalid depth (expected: 4bpp, got %i)", img->components());
		return false;
	}
	_entries.clear();
	int ncolors = img->width() * img->height();
	reserve(ncolors);
	for (int i = 0; i < ncolors; ++i) {
		const int x = i % img->width();
		const int y = i / img->width();
		add(img->colorAt(x, y));
	}
	_name = img->name();
	markDirty();
	Log::debug("Set up %i material colors", ncolors);
	return true;
}

color::RGBA ColorPalette::color(size_t index) const {
	const ColorPaletteEntry *e = entry(index);
	if (e == nullptr) {
		return _empty.color;
	}
	return e->color;
}

const palette::Material &ColorPalette::material(size_t index) const {
	const ColorPaletteEntry *e = entry(index);
	if (e == nullptr) {
		return _empty.material;
	}
	return e->material;
}

void ColorPalette::add(const color::RGBA &color, const core::String &name, const palette::Material &material) {
	ColorPaletteEntry e;
	e.color = color;
	e.name = name;
	e.material = material;
	_entries.push_back(e);
}

void ColorPalette::set(size_t index, const color::RGBA &color, const core::String &name,
					   const palette::Material &material) {
	if (index >= _entries.size()) {
		reserve(index + 256);
		setSize(index + 1);
	}
	_entries[index].color = color;
	_entries[index].name = name;
	_entries[index].material = material;
	markDirty();
}

ColorPalette::iterator ColorPalette::begin() {
	return _entries.begin();
}

ColorPalette::iterator ColorPalette::end() {
	return _entries.end();
}

ColorPalette::const_iterator ColorPalette::begin() const {
	return _entries.begin();
}

ColorPalette::const_iterator ColorPalette::end() const {
	return _entries.end();
}

core::String ColorPalette::print(const ColorPalette &palette, bool colorAsHex) {
	if (palette.size() == 0) {
		return "no colors";
	}
	core::String palStr;
	core::String line;
	for (int i = 0; i < palette.colorCount(); ++i) {
		if (i % 16 == 0 && !line.empty()) {
			palStr.append(core::String::format("%03i %s\n", i - 16, line.c_str()));
			line = "";
		}
		const core::String c = color::Color::print(palette.color(i), colorAsHex);
		line += c;
	}
	if (!line.empty()) {
		palStr.append(core::String::format("%03i %s\n", (palette.colorCount() - 1) / 16 * 16, line.c_str()));
	}
	return palStr;
}

} // namespace palette
