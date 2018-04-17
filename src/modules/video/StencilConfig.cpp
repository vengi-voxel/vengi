/**
 * @file
 */

#include "StencilConfig.h"

namespace video {

StencilConfig& StencilConfig::func(CompareFunc func) {
	_compareFunc = func;
	return *this;
}

StencilConfig& StencilConfig::failOp(StencilOp op) {
	_stencilOps[0] = op;
	return *this;
}

StencilConfig& StencilConfig::zfailOp(StencilOp op) {
	_stencilOps[1] = op;
	return *this;
}

StencilConfig& StencilConfig::zpassOp(StencilOp op) {
	_stencilOps[2] = op;
	return *this;
}

StencilConfig& StencilConfig::mask(uint32_t mask) {
	_stencilMask = mask;
	return *this;
}

}
