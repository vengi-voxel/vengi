#pragma once

#include "DefaultMarchingCubesController.h" //We'll specialise the controller contained in here

#include <limits>

namespace PolyVox {

/// This class represents a voxel storing only a density.
////////////////////////////////////////////////////////////////////////////////
/// Note that this should probably just be considered an example of how to define
/// a voxel type for the Marching Cubes algorithm. Advanced users are likely to
/// define custom voxel types and possibly custom controllers.
///
/// \sa Material, MaterialDensityPair
////////////////////////////////////////////////////////////////////////////////
template<typename Type>
class Density {
public:
	/// Constructor
	Density() :
			m_uDensity(0) {
	}

	/// Copy constructor
	Density(Type uDensity) :
			m_uDensity(uDensity) {
	}

	// The LowPassFilter uses this to convert between normal and accumulated types.
	/// Copy constructor with cast
	template<typename CastType> explicit Density(const Density<CastType>& density) {
		m_uDensity = static_cast<Type>(density.getDensity());
	}

	bool operator==(const Density& rhs) const {
		return (m_uDensity == rhs.m_uDensity);
	}

	bool operator!=(const Density& rhs) const {
		return !(*this == rhs);
	}

	// For densities we can supply mathematical operators which behave in an intuitive way.
	// In particular the ability to add and subtract densities is important in order to
	// apply an averaging filter. The ability to divide by an integer is also needed for
	// this same purpose.
	Density<Type>& operator+=(const Density<Type>& rhs) {
		m_uDensity += rhs.m_uDensity;
		return *this;
	}

	Density<Type>& operator-=(const Density<Type>& rhs) {
		m_uDensity -= rhs.m_uDensity;
		return *this;
	}

	Density<Type>& operator/=(uint32_t rhs) {
		m_uDensity /= rhs;
		return *this;
	}

	/// \return The current density of the voxel
	Type getDensity() const {
		return m_uDensity;
	}

	/**
	 * Set the density of the voxel
	 *
	 * \param uDensity The density to set to
	 */
	void setDensity(Type uDensity) {
		m_uDensity = uDensity;
	}

	/// \return The maximum allowed density of the voxel
	static Type getMaxDensity() {
		return (std::numeric_limits<Type>::max)();
	}

	/// \return The minimum allowed density of the voxel
	static Type getMinDensity() {
		return (std::numeric_limits<Type>::min)();
	}

private:
	Type m_uDensity;
};

template<typename Type>
Density<Type> operator+(const Density<Type>& lhs, const Density<Type>& rhs) {
	Density<Type> result = lhs;
	result += rhs;
	return result;
}

template<typename Type>
Density<Type> operator-(const Density<Type>& lhs, const Density<Type>& rhs) {
	Density<Type> result = lhs;
	result -= rhs;
	return result;
}

template<typename Type>
Density<Type> operator/(const Density<Type>& lhs, uint32_t rhs) {
	Density<Type> result = lhs;
	result /= rhs;
	return result;
}

// These are the predefined density types. The 8-bit types are sufficient for many purposes (including
// most games) but 16-bit and float types do have uses particularly in medical/scientific visualisation.
typedef Density<uint8_t> Density8;
typedef Density<uint16_t> Density16;
typedef Density<uint32_t> Density32;
typedef Density<float> DensityFloat;

/**
 * This is a specialisation of DefaultMarchingCubesController for the Density voxel type
 */
template<typename Type>
class DefaultMarchingCubesController<Density<Type> > {
public:
	typedef Type DensityType;
	typedef float MaterialType;

	DefaultMarchingCubesController(void) {
		// Default to a threshold value halfway between the min and max possible values.
		m_tThreshold = (Density<Type>::getMinDensity() + Density<Type>::getMaxDensity()) / 2;
	}

	DefaultMarchingCubesController(DensityType tThreshold) {
		m_tThreshold = tThreshold;
	}

	DensityType convertToDensity(Density<Type> voxel) {
		return voxel.getDensity();
	}

	MaterialType convertToMaterial(Density<Type> /*voxel*/) {
		return 1;
	}

	MaterialType blendMaterials(Density<Type> /*a*/, Density<Type> /*b*/, float /*weight*/) {
		return 1;
	}

	DensityType getThreshold(void) {
		return m_tThreshold;
	}

	void setThreshold(DensityType tThreshold) {
		m_tThreshold = tThreshold;
	}

private:
	DensityType m_tThreshold;
};

}
