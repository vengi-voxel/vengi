#include "MaterialSet.h"

#include "PolyVox/Interpolation.h"

using namespace Cubiquity;

namespace PolyVox {
MaterialSet trilerp(const MaterialSet& v000, const MaterialSet& v100, const MaterialSet& v010, const MaterialSet& v110, const MaterialSet& v001, const MaterialSet& v101,
		const MaterialSet& v011, const MaterialSet& v111, const float x, const float y, const float z) {
	core_assert((x >= 0.0f) && (y >= 0.0f) && (z >= 0.0f) && (x <= 1.0f) && (y <= 1.0f) && (z <= 1.0f));

	MaterialSet tInterpolatedValue;

	for (uint32_t ct = 0; ct < MaterialSet::getNoOfMaterials(); ct++) {
		tInterpolatedValue.setMaterial(ct,
				static_cast<uint8_t>(trilerp<float>(static_cast<float>(v000.getMaterial(ct)), static_cast<float>(v100.getMaterial(ct)), static_cast<float>(v010.getMaterial(ct)),
						static_cast<float>(v110.getMaterial(ct)), static_cast<float>(v001.getMaterial(ct)), static_cast<float>(v101.getMaterial(ct)),
						static_cast<float>(v011.getMaterial(ct)), static_cast<float>(v111.getMaterial(ct)), x, y, z)));
	}

	return tInterpolatedValue;
}
}

namespace Cubiquity {
MaterialSet operator+(const MaterialSet& lhs, const MaterialSet& rhs) {
	MaterialSet resultMat = lhs;
	resultMat += rhs;
	return resultMat;
}

MaterialSet operator-(const MaterialSet& lhs, const MaterialSet& rhs) {
	MaterialSet resultMat = lhs;
	resultMat -= rhs;
	return resultMat;
}

MaterialSet operator*(const MaterialSet& lhs, float rhs) {
	MaterialSet resultMat = lhs;
	resultMat *= rhs;
	return resultMat;
}

MaterialSet operator/(const MaterialSet& lhs, float rhs) {
	MaterialSet resultMat = lhs;
	resultMat /= rhs;
	return resultMat;
}

MaterialSetMarchingCubesController::MaterialSetMarchingCubesController(void) {
	// Default to a threshold value halfway between the min and max possible values.
	m_tThreshold = MaterialSet::getMaxMaterialValue() / 2;
}

MaterialSetMarchingCubesController::DensityType MaterialSetMarchingCubesController::convertToDensity(MaterialSet voxel) {
	return voxel.getSumOfMaterials();
}

MaterialSetMarchingCubesController::MaterialType MaterialSetMarchingCubesController::convertToMaterial(MaterialSet voxel) {
	return voxel;
}

MaterialSetMarchingCubesController::MaterialType MaterialSetMarchingCubesController::blendMaterials(MaterialSet a, MaterialSet b, float weight) {
	MaterialSet result;
	for (uint32_t ct = 0; ct < MaterialSet::getNoOfMaterials(); ct++) {
		float aFloat = static_cast<float>(a.getMaterial(ct));
		float bFloat = static_cast<float>(b.getMaterial(ct));
		float blended = (bFloat - aFloat) * weight + aFloat;
		result.setMaterial(ct, static_cast<uint8_t>(blended));
	}
	return result;
}

MaterialSetMarchingCubesController::DensityType MaterialSetMarchingCubesController::getThreshold() {
	return m_tThreshold;
}

void MaterialSetMarchingCubesController::setThreshold(DensityType tThreshold) {
	m_tThreshold = tThreshold;
}

}
