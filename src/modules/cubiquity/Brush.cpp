#include "Brush.h"

namespace Cubiquity {

Brush::Brush(float innerRadius, float outerRadius, float opacity) :
		_innerRadius(innerRadius), _outerRadius(outerRadius), _opacity(opacity) {
}

float Brush::innerRadius(void) const {
	return _innerRadius;
}

float Brush::outerRadius(void) const {
	return _outerRadius;
}

float Brush::opacity(void) const {
	return _opacity;
}

void Brush::setInnerRadius(float value) {
	_innerRadius = value;
}

void Brush::setOuterRadius(float value) {
	_outerRadius = value;
}

void Brush::setOpacity(float value) {
	_opacity = value;
}

}
