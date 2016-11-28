/**
 * @file
 */

#include "MaterialColor.h"
#include "image/Image.h"
#include "core/Singleton.h"
#include "core/App.h"
#include "io/Filesystem.h"

namespace voxel {

class MaterialColor {
private:
	image::Image _image;
	MaterialColorArray _materialColors;
	bool _initialized = false;
public:
	MaterialColor() :
			_image("**palette**") {
	}

	bool init(const io::FilePtr& file) {
		if (_initialized) {
			Log::debug("MaterialColors are already initialized");
			return true;
		}
		_initialized = true;
		if (!_image.load(file)) {
			Log::error("MaterialColors: failed to load image");
			return false;
		}
		if (!_image.isLoaded()) {
			Log::error("MaterialColors: image not fully loaded");
			return false;
		}
		const int colors = _image.width() * _image.height();
		if (colors != 256) {
			Log::error("Palette image has invalid dimensions: %i:%i", _image.width(), _image.height());
			return false;
		}
		core_assert(_image.depth() == 4);
		_materialColors.reserve(colors);
		const uint32_t* paletteData = (const uint32_t*)_image.data();
		for (int i = 0; i < colors; ++i) {
			_materialColors.emplace_back(core::Color::FromRGBA(*paletteData));
			++paletteData;
		}
		Log::info("Set up %i material colors", (int)_materialColors.size());
		return _materialColors.size() == 256u;
	}

	inline const MaterialColorArray& getColors() const {
		core_assert_msg(_initialized, "Material colors are not yet initialized");
		core_assert_msg(!_materialColors.empty(), "Failed to initialize the material colors");
		return _materialColors;
	}
};

bool initMaterialColors(const io::FilePtr& file) {
	if (!file->exists()) {
		Log::error("Failed to load %s", file->name().c_str());
		return false;
	}
	return core::Singleton<MaterialColor>::getInstance().init(file);
}

bool initDefaultMaterialColors() {
	const io::FilePtr& file = core::App::getInstance()->filesystem()->open("palette-nippon.png");
	return initMaterialColors(file);
}

const MaterialColorArray& getMaterialColors() {
	return core::Singleton<MaterialColor>::getInstance().getColors();
}

}
