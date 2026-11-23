/**
 * @file
 */

#pragma once

#include <limits>

namespace math {

constexpr float pi = 3.14159265358979323846f;
constexpr double piD = 3.14159265358979323846;
constexpr double halfPiD = 1.57079632679489661923;
constexpr double twoPiD = 6.28318530717958647692;
constexpr float halfPi = 1.57079632679489661923f;
constexpr float twoPi = 6.28318530717958647692f;
constexpr float quarterPi = 0.78539816339744830962f;
constexpr float threeOverTwoPi = 4.71238898038468985769f;
constexpr float rootTwo = 1.41421356237309504880f;
constexpr float rootThree = 1.73205080756887729353f;
constexpr float epsilonf = 1.1920928955078125e-07f;
constexpr double epsilond = 2.2204460492503131e-16;

constexpr int MaxInteger = 2147483647;
constexpr float MaxFloat = 3.402823466e+38f;
constexpr float Infinity = std::numeric_limits<float>::infinity();
constexpr double NotANumber = std::numeric_limits<double>::quiet_NaN();

}
