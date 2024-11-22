/**
 * @file
 */

#include "Material.h"
#include "core/Log.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/epsilon.hpp>

namespace palette {

Material::Material() {
	setValue(MaterialRoughness, 0.1f);
	setValue(MaterialIndexOfRefraction, 1.3f);
}

bool Material::operator!=(const Material &rhs) const {
	return !(*this == rhs);
}

bool Material::operator==(const Material &rhs) const {
	if (mask != rhs.mask) {
		return false;
	}
	if (type != rhs.type) {
		return false;
	}
	for (int i = 0; i < (int)MaterialMax - 1; ++i) {
		if (!glm::epsilonEqual(value(MaterialProperty(i)), rhs.value(MaterialProperty(i)), glm::epsilon<float>())) {
			return false;
		}
	}
	return true;
}

bool Material::has(MaterialProperty n) const {
	return (mask & (1 << n)) != 0;
}

float Material::value(MaterialProperty n) const {
	if (n == MaterialProperty::MaterialNone || n >= MaterialProperty::MaterialMax) {
		return 0.0f;
	}
	return *(&metal + (n - 1));
}

void Material::setValue(MaterialProperty n, float value) {
	if (n == MaterialProperty::MaterialNone || n >= MaterialProperty::MaterialMax) {
		return;
	}
	*(&metal + (n - 1)) = value;
	Log::debug("Material: Set %s to %f", MaterialPropertyNames[n - 1], value);
	if (value > 0.0f) {
		mask |= (1 << n);
	} else {
		mask &= ~(1 << n);
	}
}

} // namespace palette
