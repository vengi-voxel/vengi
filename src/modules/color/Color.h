/**
 * @file
 */

#pragma once

#include <glm/vec4.hpp>

namespace color {

constexpr const uint32_t magnitude = 255;
constexpr const float magnitudef = 255.0f;
constexpr const float scaleFactor = 0.7f;

const glm::vec4 &Clear();
const glm::vec4 &White();
const glm::vec4 &Black();
const glm::vec4 &LightGray();
const glm::vec4 &Gray();
const glm::vec4 &DarkGray();
const glm::vec4 &LightRed();
const glm::vec4 &Red();
const glm::vec4 &DarkRed();
const glm::vec4 &LightGreen();
const glm::vec4 &Green();
const glm::vec4 &DarkGreen();
const glm::vec4 &Lime();
const glm::vec4 &LightBlue();
const glm::vec4 &Blue();
const glm::vec4 &DarkBlue();
const glm::vec4 &SteelBlue();
const glm::vec4 &Olive();
const glm::vec4 &Pink();
const glm::vec4 &Purple();
const glm::vec4 &Yellow();
const glm::vec4 &LightYellow();
const glm::vec4 &Sandy();
const glm::vec4 &Cyan();
const glm::vec4 &Orange();
const glm::vec4 &Brown();
const glm::vec4 &LightBrown();
const glm::vec4 &DarkBrown();

}; // namespace color
