/**
 * @file
 */

#pragma once

#include "Types.h"

namespace video {

class StencilConfig {
private:
	CompareFunc _compareFunc = CompareFunc::Always;
	StencilOp _stencilOps[3] { StencilOp::Keep, StencilOp::Keep, StencilOp::Keep };
	uint32_t _stencilMask = 0xFFFFFFFFU;
	uint32_t _stencilValue = 0U;
public:
	/**
	 * The CompareFunc specifies the conditions under which a fragment passes the stencil test.
	 */
	StencilConfig& func(CompareFunc func);
	/**
	 * @brief Action to take if the stencil test fails.
	 */
	StencilConfig& failOp(StencilOp op);
	/**
	 * @brief Action to take if the stencil test is successful, but the depth test failed.
	 */
	StencilConfig& zfailOp(StencilOp op);
	/**
	 * @brief Action to take if both the stencil test and depth tests pass.
	 */
	StencilConfig& zpassOp(StencilOp op);
	/**
	 * @brief A bitwise AND operation is performed on the stencil value and reference value with this mask value before comparing them.
	 */
	StencilConfig& mask(uint32_t mask);
	/**
	 * @brief A value to compare the stencil value to using the test function.
	 */
	StencilConfig& value(uint32_t value);

	CompareFunc func() const;
	StencilOp failOp() const;
	StencilOp zfailOp() const;
	StencilOp zpassOp() const;
	uint32_t mask() const;
	uint32_t value() const;
};

inline CompareFunc StencilConfig::func() const {
	return _compareFunc;
}

inline StencilOp StencilConfig::failOp() const {
	return _stencilOps[0];
}

inline StencilOp StencilConfig::zfailOp() const {
	return _stencilOps[1];
}

inline StencilOp StencilConfig::zpassOp() const {
	return _stencilOps[2];
}

inline uint32_t StencilConfig::mask() const{
	return _stencilMask;
}

inline uint32_t StencilConfig::value() const{
	return _stencilValue;
}

}
