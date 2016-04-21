#pragma once

#include "DefaultIsQuadNeeded.h" //we'll specialise this function for this voxel type

namespace voxel {

///This class represents a voxel storing only a material.
////////////////////////////////////////////////////////////////////////////////
/// Note that this should probably just be considered an example of how to define
/// a voxel type for the Marching Cubes algorithm. Advanced users are likely to
/// define custom voxel types and possibly custom controllers.
///
/// @sa Density, MaterialDensityPair
////////////////////////////////////////////////////////////////////////////////
template<typename Type>
class Material {
public:
	Material() :
			m_uMaterial(0) {
	}
	Material(Type uMaterial) :
			m_uMaterial(uMaterial) {
	}

	bool operator==(const Material& rhs) const {
		return (m_uMaterial == rhs.m_uMaterial);
	}
	;

	bool operator!=(const Material& rhs) const {
		return !(*this == rhs);
	}

	/// @return The current material value of the voxel
	Type getMaterial() const {
		return m_uMaterial;
	}
	/**
	 * Set the material value of the voxel
	 *
	 * @param uMaterial The material to set to
	 */
	void setMaterial(Type uMaterial) {
		m_uMaterial = uMaterial;
	}

private:
	Type m_uMaterial;
};

typedef Material<uint8_t> Material8;
typedef Material<uint16_t> Material16;

template<typename Type>
class DefaultIsQuadNeeded<Material<Type> > {
public:
	bool operator()(Material<Type> back, Material<Type> front, Material<Type>& materialToUse) {
		if (back.getMaterial() > 0 && front.getMaterial() == 0) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

}
