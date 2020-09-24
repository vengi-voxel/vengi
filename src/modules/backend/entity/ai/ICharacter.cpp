/**
 * @file
 */

#include "ICharacter.h"
#include "core/GLM.h"

namespace backend {

void ICharacter::setPosition(const glm::vec3& position) {
	glm_assert_vec3(position);
	_position = position;
}

const attrib::ShadowAttributes& ICharacter::shadowAttributes() const {
	return _shadowAttributes;
}

double ICharacter::getCurrent(attrib::Type type) const {
	return _shadowAttributes.current(type);
}

double ICharacter::getMax(attrib::Type type) const {
	return _shadowAttributes.max(type);
}

void ICharacter::setCurrent(attrib::Type type, double value) {
	_shadowAttributes.setCurrent(type, value);
}

void ICharacter::setMax(attrib::Type type, double value) {
	_shadowAttributes.setMax(type, value);
}

}
