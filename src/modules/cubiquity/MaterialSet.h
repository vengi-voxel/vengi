#pragma once

#include "PolyVox/BaseVolume.h"
#include "PolyVox/MarchingCubesSurfaceExtractor.h"
#include "PolyVox/Vector.h"
#include "PolyVox/Vertex.h"
#include "core/Common.h"
#include "BitField.h"

#include <limits>

namespace Cubiquity {

class MaterialSet {
private:
	//These could be template parameters if this class needs to be templatised.
	static const uint32_t NoOfMaterials = 8;
	static const uint32_t BitsPerMaterial = 8;
	typedef uint64_t StorageType;
public:
	MaterialSet() {
		for (uint32_t ct = 0; ct < getNoOfMaterials(); ct++) {
			setMaterial(ct, 0);
		}
	}

	// This function lets us convert a Vector of floats into a MaterialSet.
	// This is useful for performing certain operations with more precision.
	MaterialSet(const ::PolyVox::Vector<NoOfMaterials, float>& value) {
		for (uint32_t ct = 0; ct < getNoOfMaterials(); ct++) {
			setMaterial(ct, static_cast<uint32_t>(value.getElement(ct) + 0.5f));
		}
	}

	// This function lets us convert a MaterialSet into a Vector of floats.
	// This is useful for performing certain operations with more precision.
	operator ::PolyVox::Vector<NoOfMaterials, float>() {
		::PolyVox::Vector<NoOfMaterials, float> result;
		for (uint32_t ct = 0; ct < getNoOfMaterials(); ct++) {
			result.setElement(ct, static_cast<float>(getMaterial(ct)));
		}

		return result;
	}

	bool operator==(const MaterialSet& rhs) const {
		for (uint32_t ct = 0; ct < getNoOfMaterials(); ct++) {
			if (getMaterial(ct) != rhs.getMaterial(ct)) {
				return false;
			}
		}
		return true;
	}
	;

	bool operator!=(const MaterialSet& rhs) const {
		return !(*this == rhs);
	}

	MaterialSet& operator+=(const MaterialSet& rhs) {
		for (uint32_t ct = 0; ct < getNoOfMaterials(); ct++) {
			float temp = static_cast<float>(getMaterial(ct));
			float rhsFloat = static_cast<float>(rhs.getMaterial(ct));
			temp += rhsFloat;
			setMaterial(ct, static_cast<uint32_t>(temp));
		}
		return *this;
	}

	MaterialSet& operator-=(const MaterialSet& rhs) {
		for (uint32_t ct = 0; ct < getNoOfMaterials(); ct++) {
			float temp = static_cast<float>(getMaterial(ct));
			float rhsFloat = static_cast<float>(rhs.getMaterial(ct));
			temp -= rhsFloat;
			setMaterial(ct, static_cast<uint32_t>(temp));
		}
		return *this;
	}

	MaterialSet& operator*=(float rhs) {
		for (uint32_t ct = 0; ct < getNoOfMaterials(); ct++) {
			float temp = static_cast<float>(getMaterial(ct));
			temp *= rhs;
			setMaterial(ct, static_cast<uint32_t>(temp));
		}
		return *this;
	}

	MaterialSet& operator/=(float rhs) {
		for (uint32_t ct = 0; ct < getNoOfMaterials(); ct++) {
			float temp = static_cast<float>(getMaterial(ct));
			temp /= rhs;
			setMaterial(ct, static_cast<uint32_t>(temp));
		}
		return *this;
	}

	static uint32_t getNoOfMaterials(void) {
		return NoOfMaterials;
	}

	static uint32_t getMaxMaterialValue(void) {
		return (0x01 << BitsPerMaterial) - 1;
	}

	uint32_t getMaterial(uint32_t index) const {
		core_assert(index < getNoOfMaterials());

		// We store the materials with material 0 in the LSBs and materials N in the MSBs. This feels slightly counter-intutive
		// (like reading right-to-left) when looking at the order of bytes on paper, but in memory material 0 will be stored
		// at a lower address and material N will be stored at a higher address (assuming a little-endien system). This maps
		// more closely to how an array would be laid out, making it easier to switch to that in the future if we want to.

		return static_cast<uint32_t>(mWeights.getBits(index * 8 + 7, index * 8));

		// Move the required bits into the least significant bits of result.
		/*StorageType result = mMaterials >> (BitsPerMaterial * index);

		 // Build a mask containing all '0's except for the least significant bits (which are '1's).
		 StorageType mask = (std::numeric_limits<StorageType>::max)(); //Set to all '1's
		 mask = mask << BitsPerMaterial; // Insert the required number of '0's for the lower bits
		 mask = ~mask; // And invert
		 result = result & mask;

		 return static_cast<uint32_t>(result);*/
	}

	void setMaterial(uint32_t index, uint32_t value) {
		core_assert(index < getNoOfMaterials());

		// We store the materials with material 0 in the LSBs and materials N in the MSBs. This feels slightly counter-intutive
		// (like reading right-to-left) when looking at the order of bytes on paper, but in memory material 0 will be stored
		// at a lower address and material N will be stored at a higher address (assuming a little-endien system). This maps
		// more closely to how an array would be laid out, making it easier to switch to that in the future if we want to.

		mWeights.setBits(index * 8 + 7, index * 8, value);

		// The bits we want to set first get cleared to zeros.
		// To do this we create a mask which is all '1' except
		// for the bits we wish to clear (which are '0').
		/*StorageType mask = (std::numeric_limits<StorageType>::max)(); //Set to all '1's
		 mask = mask << BitsPerMaterial; // Insert the required number of '0's for the lower bits
		 mask = ~mask; // We want to insert '1's next, so fake this by inverting before and after
		 mask = mask << (BitsPerMaterial * index); // Insert the '0's which we will invert to '1's.
		 mask = ~mask; // And invert back again

		 // Clear the bits which we're about to set.
		 mMaterials &= mask;

		 // OR with the value to set the bits
		 StorageType temp = value;
		 temp = temp << (BitsPerMaterial * index);
		 mMaterials |= temp;*/
	}

	uint32_t getSumOfMaterials(void) const {
		uint32_t sum = 0;
		for (uint32_t ct = 0; ct < getNoOfMaterials(); ct++) {
			sum += getMaterial(ct);
		}
		return sum;
	}

	// This function attempts to adjust the values of the different materials so that they sum to
	// the required value. It does this while attempting to preserve the existing material ratios.
	/*void setSumOfMaterials(uint32_t targetSum)
	 {
	 uint32_t initialSum = getSumOfMaterials();

	 if(initialSum == 0)
	 {
	 }
	 else
	 {
	 float scaleFactor = static_cast<float>(targetSum) / static_cast<float>(initialSum);
	 PolyVox::Vector<NoOfMaterials, float> ideal = *this;
	 ideal *= scaleFactor;
	 *this = ideal;
	 }
	 }*/

	void clampSumOfMaterials(void) {
		uint32_t initialSum = getSumOfMaterials();
		if (initialSum > getMaxMaterialValue()) {
			uint32_t excess = initialSum - getMaxMaterialValue();
			uint32_t nextMatToReduce = 0;
			while (excess) {
				uint32_t material = getMaterial(nextMatToReduce);

				// Don't reduce if it's aready zero - it will wrap around and become big.
				if (material > 0) {
					material--;
					setMaterial(nextMatToReduce, material);

					excess--;
				}

				nextMatToReduce++;
				nextMatToReduce %= NoOfMaterials;
			}
		}

		core_assert_msg(getSumOfMaterials() <= getMaxMaterialValue(), "MaterialSet::clampSum() failed to perform clamping");
	}

public:
	BitField<StorageType> mWeights;
};

class MaterialSetMarchingCubesController {
public:
	typedef uint8_t DensityType;
	typedef MaterialSet MaterialType;

	MaterialSetMarchingCubesController(void);

	DensityType convertToDensity(MaterialSet voxel);
	MaterialType convertToMaterial(MaterialSet voxel);

	MaterialType blendMaterials(MaterialSet a, MaterialSet b, float weight);

	DensityType getThreshold(void);

	void setThreshold(DensityType tThreshold);

private:
	DensityType m_tThreshold;
};

typedef ::PolyVox::MarchingCubesVertex<MaterialSet> TerrainVertex;
typedef ::PolyVox::Mesh<TerrainVertex, uint16_t> TerrainMesh;

}

// We overload the trilinear interpolation for the MaterialSet type because it does not have enough precision.
// The overloaded version converts the values to floats and interpolates those before converting back.
// See also http://www.gotw.ca/publications/mill17.htm - Why Not Specialize Function Templates?
namespace PolyVox {
::Cubiquity::MaterialSet trilerp(const ::Cubiquity::MaterialSet& v000, const ::Cubiquity::MaterialSet& v100, const ::Cubiquity::MaterialSet& v010,
		const ::Cubiquity::MaterialSet& v110, const ::Cubiquity::MaterialSet& v001, const ::Cubiquity::MaterialSet& v101, const ::Cubiquity::MaterialSet& v011,
		const ::Cubiquity::MaterialSet& v111, const float x, const float y, const float z);
}
