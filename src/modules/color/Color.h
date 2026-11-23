#include "math/Vec4f.h"
/**
 * @file
 */

#pragma once


namespace color {

constexpr const uint32_t magnitude = 255;
constexpr const float magnitudef = 255.0f;
constexpr const float scaleFactor = 0.7f;

const math::Vec4f &Clear();
const math::Vec4f &White();
const math::Vec4f &Black();
const math::Vec4f &LightGray();
const math::Vec4f &Gray();
const math::Vec4f &DarkGray();
const math::Vec4f &LightRed();
const math::Vec4f &Red();
const math::Vec4f &DarkRed();
const math::Vec4f &LightGreen();
const math::Vec4f &Green();
const math::Vec4f &DarkGreen();
const math::Vec4f &Lime();
const math::Vec4f &LightBlue();
const math::Vec4f &Blue();
const math::Vec4f &DarkBlue();
const math::Vec4f &SteelBlue();
const math::Vec4f &Olive();
const math::Vec4f &Pink();
const math::Vec4f &Purple();
const math::Vec4f &Yellow();
const math::Vec4f &LightYellow();
const math::Vec4f &Sandy();
const math::Vec4f &Cyan();
const math::Vec4f &Orange();
const math::Vec4f &Brown();
const math::Vec4f &LightBrown();
const math::Vec4f &DarkBrown();

}; // namespace color
