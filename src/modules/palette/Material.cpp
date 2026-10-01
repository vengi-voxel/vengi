/**
 * @file
 */

#include "Material.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/epsilon.hpp>

namespace palette {

Material::Material() {
	setValue(MaterialRoughness, 0.1f);
	setValue(MaterialIndexOfRefraction, 1.3f);
}

bool materialTypeFromName(const char *name, MaterialType &out) {
	if (name == nullptr) {
		return false;
	}
	for (int i = 0; i < lengthof(MaterialTypeNames); ++i) {
		if (core::string::iequals(name, MaterialTypeNames[i])) {
			out = (MaterialType)i;
			return true;
		}
	}
	return false;
}

void applyMaterialTypeDefaults(Material &mat) {
	const MaterialType type = mat.type;
	mat = Material();
	mat.type = type;
	switch (type) {
	case MaterialType::Diffuse:
		break;
	case MaterialType::Metal:
		mat.setValue(MaterialMetal, 1.0f);
		mat.setValue(MaterialRoughness, 0.2f);
		mat.setValue(MaterialSpecular, 1.0f);
		break;
	case MaterialType::Glass:
		mat.setValue(MaterialIndexOfRefraction, 1.5f);
		mat.setValue(MaterialRoughness, 0.0f);
		break;
	case MaterialType::Emit:
		mat.setValue(MaterialEmit, 1.0f);
		break;
	case MaterialType::Blend:
		mat.setValue(MaterialIndexOfRefraction, 1.5f);
		mat.setValue(MaterialRoughness, 0.0f);
		break;
	case MaterialType::Media:
		mat.setValue(MaterialIndexOfRefraction, 1.01f);
		mat.setValue(MaterialDensity, 0.5f);
		mat.setValue(MaterialPhase, 0.5f);
		mat.setValue(MaterialMedia, 1.0f);
		mat.setValue(MaterialRoughness, 1.0f);
		break;
	}
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
	Log::trace("Material: Set %s to %f", MaterialPropertyNames[n - 1], value);
	if (value > 0.0f) {
		mask |= (1 << n);
	} else {
		mask &= ~(1 << n);
	}
}

} // namespace palette
