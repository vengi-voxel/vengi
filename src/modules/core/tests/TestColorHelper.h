/**
 * @file
 */

#pragma once

#include "color/Color.h"
#include "color/RGBA.h"
#include <gtest/gtest.h>
#include <iomanip>

namespace core {

inline ::std::ostream &operator<<(::std::ostream &os, const core::RGBA &color) {
	return os << core::Color::print(color).c_str();
}

namespace priv {

inline testing::AssertionResult CmpHelperRGBAEQ(const char *lhs_expression, const char *rhs_expression,
										 const char *max_distance_expression, core::RGBA lhs_value, core::RGBA rhs_value,
										 float max_distance) {
	const float actual_distance = core::Color::getDistance(lhs_value, rhs_value, core::Color::Distance::HSB);
	if (actual_distance <= max_distance) {
		return testing::AssertionSuccess();
	}

	testing::Message msg;
	msg << std::fixed << std::setprecision(7);
	msg << "Expected similar colors:";
	msg << "\n  " << lhs_expression;
	msg << "\n    Which is: " << core::Color::print(lhs_value, true).c_str();
	msg << "\n  " << rhs_expression;
	msg << "\n    Which is: " << core::Color::print(rhs_value, true).c_str();
	msg << "\n  With a distance of " << actual_distance << " (max allowed would have been "
		<< max_distance << ", which is a delta of " << (actual_distance - max_distance) << ")";

	return testing::AssertionFailure() << msg;
}
} // namespace priv
} // namespace core

#define EXPECT_COLOR_NEAR(rgba1, rgba2, delta) EXPECT_PRED_FORMAT3(::core::priv::CmpHelperRGBAEQ, rgba1, rgba2, delta)
