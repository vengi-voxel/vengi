/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace ui {
namespace font {

/**
 * @brief Return paths to all system-installed font files (.ttf, .ttc, .otf).
 *
 * Platform-independent: scans standard font directories on Linux, macOS and Windows.
 */
core::DynamicArray<core::String> findSystemFonts();

/**
 * @brief Filter and order system fonts by fallback priority for CJK / non-Latin glyphs.
 * @param[in] systemFonts List of candidate font paths discovered on the system.
 * @param[in] language Optional locale/language string (e.g. "ja_JP", "zh_CN", "zh_TW", "ko_KR").
 */
core::DynamicArray<core::String> selectFallbackSystemFonts(const core::DynamicArray<core::String> &systemFonts,
														   const core::String &language = core::String::Empty);

} // namespace font
} // namespace ui
