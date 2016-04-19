#pragma once

#include "DefaultIsQuadNeeded.h" //we'll specialise this function for this voxel type
#include "DefaultMarchingCubesController.h" //We'll specialise the controller contained in here

namespace PolyVox {

/// This class represents a voxel storing only a density.
////////////////////////////////////////////////////////////////////////////////
/// Note that this should probably just be considered an example of how to define
/// a voxel type for the Marching Cubes algorithm. Advanced users are likely to
/// define custom voxel types and possibly custom controllers.
///
/// @sa Density, Material
////////////////////////////////////////////////////////////////////////////////
template<typename Type, uint8_t NoOfMaterialBits, uint8_t NoOfDensityBits>
class MaterialDensityPair {
public:
	MaterialDensityPair() :
			m_uMaterial(0), m_uDensity(0) {
	}

	MaterialDensityPair(Type uMaterial, Type uDensity) :
			m_uMaterial(uMaterial), m_uDensity(uDensity) {
	}

	bool operator==(const MaterialDensityPair& rhs) const {
		return (m_uMaterial == rhs.m_uMaterial) && (m_uDensity == rhs.m_uDensity);
	}

	bool operator!=(const MaterialDensityPair& rhs) const {
		return !(*this == rhs);
	}

	MaterialDensityPair<Type, NoOfMaterialBits, NoOfDensityBits>& operator+=(const MaterialDensityPair<Type, NoOfMaterialBits, NoOfDensityBits>& rhs) {
		m_uDensity += rhs.m_uDensity;

		// What should we do with the material? Conceptually the idea of adding materials makes no sense, but for our
		// purposes we consider the 'sum' of two materials to just be the max. At least this way it is commutative.
		m_uMaterial = (std::max)(m_uMaterial, rhs.m_uMaterial);

		return *this;
	}

	MaterialDensityPair<Type, NoOfMaterialBits, NoOfDensityBits>& operator/=(uint32_t rhs) {
		// There's nothing sensible we can do with the material, so this function only affects the density.
		m_uDensity /= rhs;
		return *this;
	}

	Type getDensity() const {
		return m_uDensity;
	}

	Type getMaterial() const {
		return m_uMaterial;
	}

	void setDensity(Type uDensity) {
		m_uDensity = uDensity;
	}

	void setMaterial(Type uMaterial) {
		m_uMaterial = uMaterial;
	}

	static Type getMaxDensity() {
		return (0x01 << NoOfDensityBits) - 1;
	}

	static Type getMinDensity() {
		return 0;
	}

private:
	Type m_uMaterial :NoOfMaterialBits;
	Type m_uDensity :NoOfDensityBits;
};

template<typename Type, uint8_t NoOfMaterialBits, uint8_t NoOfDensityBits>
class DefaultIsQuadNeeded<MaterialDensityPair<Type, NoOfMaterialBits, NoOfDensityBits> > {
public:
	bool operator()(MaterialDensityPair<Type, NoOfMaterialBits, NoOfDensityBits> back, MaterialDensityPair<Type, NoOfMaterialBits, NoOfDensityBits> front,
			MaterialDensityPair<Type, NoOfMaterialBits, NoOfDensityBits>& materialToUse) {
		if (back.getMaterial() > 0 && front.getMaterial() == 0) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

typedef MaterialDensityPair<uint8_t, 4, 4> MaterialDensityPair44;
typedef MaterialDensityPair<uint16_t, 8, 8> MaterialDensityPair88;

}
