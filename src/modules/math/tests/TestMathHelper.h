/**
 * @file
 */

#pragma once

#include <glm/vector_relational.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <gtest/gtest.h>
#include <iomanip>

namespace glm {
namespace priv {

template<int L, typename T, qualifier Q = defaultp>
inline testing::AssertionResult AssertMsg(const char *lhs_expression, const char *rhs_expression,
										  const glm::vec<L, T, Q> &lhs_value, const glm::vec<L, T, Q> &rhs_value) {
	testing::Message msg;
	msg << std::fixed << std::setprecision(7);
	msg << "Expected similar values: ";
	for (int i = 0; i < lhs_value.length(); i++) {
		if (i > 0) {
			msg << ", ";
		} else {
			msg << "\n  " << lhs_expression << ": ";
		}
		msg << lhs_value[i];
	}
	for (int i = 0; i < rhs_value.length(); i++) {
		if (i > 0) {
			msg << ", ";
		} else {
			msg << "\n  " << rhs_expression << ": ";
		}
		msg << rhs_value[i];
	}

	return testing::AssertionFailure() << msg;
}

template<int L, typename T, qualifier Q = defaultp>
inline testing::AssertionResult CmpHelperVecEQ(const char *lhs_expression, const char *rhs_expression,
											   const char *max_distance_expression, const glm::vec<L, T, Q> &lhs_value,
											   const glm::vec<L, T, Q> &rhs_value, T max_distance) {
	if (glm::all(glm::epsilonEqual(lhs_value, rhs_value, max_distance))) {
		return testing::AssertionSuccess();
	}

	return AssertMsg(lhs_expression, rhs_expression, lhs_value, rhs_value);
}

template<int L, typename T, qualifier Q = defaultp>
inline testing::AssertionResult CmpHelperVecNE(const char *lhs_expression, const char *rhs_expression,
											   const char *max_distance_expression, const glm::vec<L, T, Q> &lhs_value,
											   const glm::vec<L, T, Q> &rhs_value, T max_distance) {
	if (glm::any(glm::epsilonNotEqual(lhs_value, rhs_value, max_distance))) {
		return testing::AssertionSuccess();
	}

	return AssertMsg(lhs_expression, rhs_expression, lhs_value, rhs_value);
}

} // namespace priv
} // namespace glm

namespace glm {

inline ::std::ostream &operator<<(::std::ostream &os, const vec3 &v) {
	os << std::fixed << std::setprecision(7);
	for (int i = 0; i < v.length(); i++) {
		if (i > 0) {
			os << ", ";
		}
		os << v[i];
	}
	return os;
}

inline ::std::ostream &operator<<(::std::ostream &os, const ivec3 &v) {
	for (int i = 0; i < v.length(); i++) {
		if (i > 0) {
			os << ", ";
		}
		os << v[i];
	}
	return os;
}
inline ::std::ostream &operator<<(::std::ostream &os, const vec4 &v) {
	os << std::fixed << std::setprecision(7);
	for (int i = 0; i < v.length(); i++) {
		if (i > 0) {
			os << ", ";
		}
		os << v[i];
	}
	return os;
}

inline ::std::ostream &operator<<(::std::ostream &os, const ivec4 &v) {
	for (int i = 0; i < v.length(); i++) {
		if (i > 0) {
			os << ", ";
		}
		os << v[i];
	}
	return os;
}

inline ::std::ostream &operator<<(::std::ostream &os, const vec2 &v) {
	for (int i = 0; i < v.length(); i++) {
		if (i > 0) {
			os << ", ";
		}
		os << v[i];
	}
	return os;
}

inline ::std::ostream &operator<<(::std::ostream &os, const ivec2 &v) {
	for (int i = 0; i < v.length(); i++) {
		if (i > 0) {
			os << ", ";
		}
		os << v[i];
	}
	return os;
}

}


#define EXPECT_VEC_NEAR(lhs, rhs, delta) EXPECT_PRED_FORMAT3(::glm::priv::CmpHelperVecEQ, lhs, rhs, delta)
#define ASSERT_VEC_NEAR(lhs, rhs, delta) ASSERT_PRED_FORMAT3(::glm::priv::CmpHelperVecEQ, lhs, rhs, delta)
#define EXPECT_VEC_NOT_NEAR(lhs, rhs, delta) EXPECT_PRED_FORMAT3(::glm::priv::CmpHelperVecNE, lhs, rhs, delta)
#define ASSERT_VEC_NOT_NEAR(lhs, rhs, delta) ASSERT_PRED_FORMAT3(::glm::priv::CmpHelperVecNE, lhs, rhs, delta)
#define EXPECT_MAT_NEAR(lhs, rhs, delta) \
	do { \
		for (int col = 0; col < (lhs).length(); col++) { \
			EXPECT_VEC_NEAR((lhs)[col], (rhs)[col], delta); \
		} \
	} while (0)
