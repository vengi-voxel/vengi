#pragma once

#include "BaseVolume.h"

#include <limits>

namespace PolyVox {

/**
 * This class provides a default implementation of a controller for the MarchingCubesSurfaceExtractor. It controls the behaviour of the
 * MarchingCubesSurfaceExtractor and provides the required properties from the underlying voxel type.
 *
 * PolyVox does not enforce any requirements regarding what data must be present in a voxel, and instead allows any primitive or user-defined
 * type to be used. However, the Marching Cubes algorithm does have some requirents about the underlying data in that conceptually it operates
 * on a <i>density field</i>. In addition, the PolyVox implementation of the Marching Cubes algorithm also understands the idea of each voxel
 * having a material which is copied into the vertex data.
 *
 * Because we want the MarchingCubesSurfaceExtractor to work on <i>any</i> voxel type, we use a <i>Marching Cubes controller</i> (passed as
 * a parameter of the MarchingCubesSurfaceExtractor) to expose the required properties. This parameter defaults to the DefaultMarchingCubesController.
 * The main implementation of this class is designed to work with primitives data types, and the class is also specialised for the Material,
 * Density and MaterialdensityPair classes.
 *
 * If you create a custom class for your voxel data then you probably want to include a specialisation of DefaultMarchingCubesController,
 * though you don't have to if you don't want to use the Marching Cubes algorithm or if you prefer to define a seperate Marching Cubes controller
 * and pass it as an explicit parameter (rather than relying on the default).
 *
 * For primitive types, the DefaultMarchingCubesController considers the value of the voxel to represent it's density and just returns a constant
 * for the material. So you can, for example, run the MarchingCubesSurfaceExtractor on a volume of floats or ints.
 *
 * It is possible to customise the behaviour of the controller by providing a threshold value through the constructor. The extracted surface
 * will pass through the density value specified by the threshold, and so you should make sure that the threshold value you choose is between
 * the minimum and maximum values found in your volume data. By default it is in the middle of the representable range of the underlying type.
 *
 * @sa extractMarchingCubesMesh
 *
 */
template<typename VoxelType>
class DefaultMarchingCubesController {
public:
	/// Used to inform the MarchingCubesSurfaceExtractor about which type it should use for representing densities.
	typedef VoxelType DensityType;
	/// Used to inform the MarchingCubesSurfaceExtractor about which type it should use for representing materials.
	typedef VoxelType MaterialType;

	/**
	 * Constructor
	 *
	 * This version of the constructor takes no parameters and sets the threshold to the middle of the representable range of the underlying type.
	 * For example, if the voxel type is 'uint8_t' then the representable range is 0-255, and the threshold will be set to 127. On the other hand,
	 * if the voxel type is 'float' then the representable range is -FLT_MAX to FLT_MAX and the threshold will be set to zero.
	 */
	DefaultMarchingCubesController() {
		if (std::is_signed<DensityType>()) {
			m_tThreshold = DensityType(0);
		} else {
			m_tThreshold = (((std::numeric_limits<DensityType>::min)() + (std::numeric_limits<DensityType>::max)()) / 2);
		}
	}

	/**
	 * Converts the underlying voxel type into a density value.
	 *
	 * The default implementation of this function just returns the voxel type directly and is suitable for primitives types. Specialisations of
	 * this class can modify this behaviour.
	 */
	DensityType convertToDensity(VoxelType voxel) {
		return voxel;
	}

	/**
	 * Converts the underlying voxel type into a material value.
	 *
	 * The default implementation of this function just returns the constant '1'. There's not much else it can do, as it needs to work with primitive
	 * types and the actual value of the type is already being considered to be the density. Specialisations of this class can modify this behaviour.
	 */
	MaterialType convertToMaterial(VoxelType /*voxel*/) {
		return 1;
	}

	/**
	 * Returns a material which is in some sense a weighted combination of the supplied materials.
	 *
	 * The Marching Cubes algotithm generates vertices which lie between voxels, and ideally the material of the vertex should be interpolated from the materials
	 * of the voxels. In practice, that material type is often an integer identifier (e.g. 1 = rock, 2 = soil, 3 = grass) and an interpolation doean't make sense
	 * (e.g. soil is not a combination or rock and grass). Therefore this default interpolation just returns whichever material is associated with a voxel of the
	 * higher density, but if more advanced voxel types do support interpolation then it can be implemented in this function.
	 */
	MaterialType blendMaterials(VoxelType a, VoxelType b, float /*weight*/) {
		if (convertToDensity(a) > convertToDensity(b)) {
			return convertToMaterial(a);
		}
		return convertToMaterial(b);
	}

	/**
	 * Returns the density value which was passed to the constructor.
	 *
	 * As mentioned in the class description, the extracted surface will pass through the density value specified by the threshold, and so you
	 * should make sure that the threshold value you choose is between the minimum and maximum values found in your volume data. By default it
	 * is in the middle of the representable range of the underlying type.
	 */
	DensityType getThreshold() {
		return m_tThreshold;
	}

	void setThreshold(DensityType tThreshold) {
		m_tThreshold = tThreshold;
	}

private:
	DensityType m_tThreshold;
};

}
