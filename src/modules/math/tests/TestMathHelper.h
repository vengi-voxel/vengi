/**
 * @file
 */

#pragma once

#include "math/Print.h"
#include "math/Vec2f.h"
#include "math/Vec2i.h"
#include "math/Vec3f.h"
#include "math/Vec3i.h"
#include "math/Vec4f.h"
#include "math/Vec4i.h"
#include <gtest/gtest.h>

template<class T>
inline ::testing::AssertionResult vecComponentsNear(const T &lhs, const T &rhs, double delta) {
	for (int i = 0; i < lhs.length(); ++i) {
		const double a = (double)lhs[i];
		const double b = (double)rhs[i];
		const double diff = a > b ? a - b : b - a;
		if (diff > delta) {
			return ::testing::AssertionFailure() << "component " << i << ": " << a << " vs " << b;
		}
	}
	return ::testing::AssertionSuccess();
}

#define EXPECT_VEC_NEAR(lhs, rhs, delta) EXPECT_TRUE(vecComponentsNear((lhs), (rhs), (double)(delta)))

#define ASSERT_VEC_NEAR(lhs, rhs, delta) ASSERT_TRUE(vecComponentsNear((lhs), (rhs), (double)(delta)))

#define EXPECT_VEC_NOT_NEAR(lhs, rhs, delta)                                                                                   \
	do {                                                                                                                       \
		const auto &_vl = (lhs);                                                                                              \
		const auto &_vr = (rhs);                                                                                              \
		bool _diff = false;                                                                                                   \
		for (int _i = 0; _i < _vl.length(); ++_i) {                                                                            \
			const double _d = (double)_vl[_i] - (double)_vr[_i];                                                           \
			if (_d > (double)(delta) || -_d > (double)(delta)) {                                                           \
				_diff = true;                                                                                                 \
			}                                                                                                                 \
		}                                                                                                                      \
		EXPECT_TRUE(_diff);                                                                                                   \
	} while (0)

#define ASSERT_VEC_NOT_NEAR(lhs, rhs, delta) EXPECT_VEC_NOT_NEAR(lhs, rhs, delta)

#define EXPECT_MAT_NEAR(lhs, rhs, delta)                                                                                       \
	do {                                                                                                                       \
		for (int col = 0; col < (lhs).length(); col++) {                                                                       \
			EXPECT_VEC_NEAR((lhs)[col], (rhs)[col], delta);                                                                    \
		}                                                                                                                      \
	} while (0)
