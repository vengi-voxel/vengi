/**
 * @file
 */

#pragma once

#include "core/DirtyState.h"
#include "color/RGBA.h"
#include "core/SharedPtr.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "palette/Material.h"

namespace image {
class Image;
}

namespace palette {

struct ColorPaletteEntry {
	color::RGBA color;
	core::String name;
	palette::Material material;
};

class ColorPalette : public core::DirtyState {
private:
	core::DynamicArray<ColorPaletteEntry> _entries;

	const ColorPaletteEntry *entry(size_t index) const;
	ColorPaletteEntry _empty;
	core::String _name;
	core::String _filename;

public:
	void setSize(size_t size);

	void reserve(size_t size);

	void setColor(size_t index, const color::RGBA &color);

	void setColorName(size_t index, const core::String &name);

	void setMaterial(size_t index, const palette::Material &material);

	void setName(const core::String &name);
	void setFilename(const core::String &filename);
	const core::String &name() const;
	const core::String &filename() const;

	int colorCount() const;

	/**
	 * @brief Remove duplicated or full transparent colors
	 */
	void optimize();

	size_t size() const;

	const core::String &colorName(size_t index) const;

	bool load(const core::SharedPtr<image::Image> &img);

	color::RGBA color(size_t index) const;

	const palette::Material &material(size_t index) const;

	void add(const color::RGBA &color, const core::String &name = core::String::Empty,
			 const palette::Material &material = {});
	void set(size_t index, const color::RGBA &color, const core::String &name = core::String::Empty,
			 const palette::Material &material = {});

	using iterator = core::DynamicArray<ColorPaletteEntry>::iterator;
	using const_iterator = core::DynamicArray<ColorPaletteEntry>::const_iterator;

	iterator begin();

	iterator end();

	const_iterator begin() const;

	const_iterator end() const;

	static core::String print(const ColorPalette &palette, bool colorAsHex = false);
};

inline const core::String &ColorPalette::filename() const {
	return _filename;
}

inline void ColorPalette::setFilename(const core::String &filename) {
	_filename = filename;
}

} // namespace palette
