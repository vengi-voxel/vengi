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

} // namespace font
} // namespace ui
