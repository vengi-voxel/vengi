/**
 * @file
 */

#include "ui/font/FontResolver.h"

#ifdef __EMSCRIPTEN__

namespace ui {
namespace font {

core::DynamicArray<core::String> findSystemFonts() {
	return {};
}

} // namespace font
} // namespace ui

#endif // __EMSCRIPTEN__
