/**
 * @file
 */

#include "core/DirtyState.h"
#include "core/RGBA.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "palette/Material.h"

namespace palette {

struct ColorPaletteEntry {
	core::RGBA color;
	core::String name;
	palette::Material material;
};

class ColorPalette : public core::DirtyState {
private:
	core::DynamicArray<ColorPaletteEntry> _entries;

	const ColorPaletteEntry *entry(size_t index) const {
		if (index >= _entries.size()) {
			return nullptr;
		}
		return &_entries[index];
	}
	ColorPaletteEntry _empty;

	core::String _name;

public:
	void setSize(size_t size) {
		_entries.resize(size);
	}

	void setColor(size_t index, const core::RGBA &color) {
		if (index >= _entries.size()) {
			return;
		}
		_entries[index].color = color;
	}

	void setColorName(size_t index, const core::String &name) {
		if (index >= _entries.size()) {
			return;
		}
		_entries[index].name = name;
	}

	void setMaterial(size_t index, const palette::Material &material) {
		if (index >= _entries.size()) {
			return;
		}
		_entries[index].material = material;
	}

	void setName(const core::String &name) {
		_name = name;
	}

	int colorCount() const {
		return (int)_entries.size();
	}

	const core::String &name() const {
		return _name;
	}

	size_t size() const {
		return _entries.size();
	}

	const core::String &colorName(size_t index) const {
		const ColorPaletteEntry *e = entry(index);
		if (e == nullptr) {
			return _empty.name;
		}
		return e->name;
	}

	core::RGBA color(size_t index) const {
		const ColorPaletteEntry *e = entry(index);
		if (e == nullptr) {
			return _empty.color;
		}
		return e->color;
	}

	const palette::Material &material(size_t index) const {
		const ColorPaletteEntry *e = entry(index);
		if (e == nullptr) {
			return _empty.material;
		}
		return e->material;
	}

	void add(const core::RGBA &color, const core::String &name = core::String::Empty,
			 const palette::Material &material = {}) {
		ColorPaletteEntry e;
		e.color = color;
		e.name = name;
		e.material = material;
		_entries.push_back(e);
	}

	using iterator = core::DynamicArray<ColorPaletteEntry>::iterator;
	using const_iterator = core::DynamicArray<ColorPaletteEntry>::const_iterator;

	iterator begin() {
		return _entries.begin();
	}

	iterator end() {
		return _entries.end();
	}

	const_iterator begin() const {
		return _entries.begin();
	}

	const_iterator end() const {
		return _entries.end();
	}
};

} // namespace palette
