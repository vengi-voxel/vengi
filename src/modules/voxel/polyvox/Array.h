#pragma once

#include "core/Common.h"

#include <cstdint>

namespace voxel {

template<uint32_t noOfDims, typename ElementType>
class Array {
public:
	Array(uint32_t width) :
			m_pElements(0) {
		static_assert(noOfDims == 1, "This constructor can only be used with a one-dimensional array");
		m_uDimensions[0] = width;

		initialize();
	}

	Array(uint32_t width, uint32_t height) :
			m_pElements(0) {
		static_assert(noOfDims == 2, "This constructor can only be used with a two-dimensional array");
		m_uDimensions[0] = width;
		m_uDimensions[1] = height;

		initialize();
	}

	Array(uint32_t width, uint32_t height, uint32_t depth) :
			m_pElements(0) {
		static_assert(noOfDims == 3, "This constructor can only be used with a three-dimensional array");
		m_uDimensions[0] = width;
		m_uDimensions[1] = height;
		m_uDimensions[2] = depth;

		initialize();
	}

	// These are deleted to avoid accidental copying.
	Array<noOfDims, ElementType>(const Array<noOfDims, ElementType>&) = delete;
	Array<noOfDims, ElementType>& operator=(const Array<noOfDims, ElementType>&) = delete;

	~Array() {
		delete[] m_pElements;
	}

	ElementType& operator()(uint32_t x) const {
		static_assert(noOfDims == 1, "This accessor can only be used with a one-dimensional array");
		core_assert_msg(x < m_uDimensions[0], "Array access is out-of-range.");
		return m_pElements[x];
	}

	ElementType& operator()(uint32_t x, uint32_t y) const {
		static_assert(noOfDims == 2, "This accessor can only be used with a two-dimensional array");
		core_assert_msg(x < m_uDimensions[0] && y < m_uDimensions[1], "Array access is out-of-range.");
		return m_pElements[y * m_uDimensions[0] + x];
	}

	ElementType& operator()(uint32_t x, uint32_t y, uint32_t z) const {
		static_assert(noOfDims == 3, "This accessor can only be used with a three-dimensional array");
		core_assert_msg(x < m_uDimensions[0] && y < m_uDimensions[1] && z < m_uDimensions[2], "Array access is out-of-range.");
		return m_pElements[z * m_uDimensions[0] * m_uDimensions[1] + y * m_uDimensions[0] + x];
	}

	uint32_t getDimension(uint32_t dimension) const {
		return m_uDimensions[dimension];
	}

	ElementType* getRawData() {
		return m_pElements;
	}

	uint32_t getNoOfElements() {
		return m_uNoOfElements;
	}

	void swap(Array& other) {
		ElementType* temp = other.m_pElements;
		other.m_pElements = m_pElements;
		m_pElements = temp;
	}

private:

	void initialize() {
		// Calculate the total number of elements in the array.
		m_uNoOfElements = 1;
		for (uint32_t i = 0; i < noOfDims; i++) {
			m_uNoOfElements *= m_uDimensions[i];
		}
		m_pElements = new ElementType[m_uNoOfElements];
	}

	uint32_t m_uDimensions[noOfDims];
	uint32_t m_uNoOfElements;
	ElementType* m_pElements;
};

///A 1D Array of floats.
typedef Array<1, float> Array1DFloat;
///A 1D Array of doubles.
typedef Array<1, double> Array1DDouble;
///A 1D Array of signed 8-bit values.
typedef Array<1, int8_t> Array1DInt8;
///A 1D Array of unsigned 8-bit values.
typedef Array<1, uint8_t> Array1DUint8;
///A 1D Array of signed 16-bit values.
typedef Array<1, int16_t> Array1DInt16;
///A 1D Array of unsigned 16-bit values.
typedef Array<1, uint16_t> Array1DUint16;
///A 1D Array of signed 32-bit values.
typedef Array<1, int32_t> Array1DInt32;
///A 1D Array of unsigned 32-bit values.
typedef Array<1, uint32_t> Array1DUint32;

///A 2D Array of floats.
typedef Array<2, float> Array2DFloat;
///A 2D Array of doubles.
typedef Array<2, double> Array2DDouble;
///A 2D Array of signed 8-bit values.
typedef Array<2, int8_t> Array2DInt8;
///A 2D Array of unsigned 8-bit values.
typedef Array<2, uint8_t> Array2DUint8;
///A 2D Array of signed 16-bit values.
typedef Array<2, int16_t> Array2DInt16;
///A 2D Array of unsigned 16-bit values.
typedef Array<2, uint16_t> Array2DUint16;
///A 2D Array of signed 32-bit values.
typedef Array<2, int32_t> Array2DInt32;
///A 2D Array of unsigned 32-bit values.
typedef Array<2, uint32_t> Array2DUint32;

///A 3D Array of floats.
typedef Array<3, float> Array3DFloat;
///A 3D Array of doubles.
typedef Array<3, double> Array3DDouble;
///A 3D Array of signed 8-bit values.
typedef Array<3, int8_t> Array3DInt8;
///A 3D Array of unsigned 8-bit values.
typedef Array<3, uint8_t> Array3DUint8;
///A 3D Array of signed 16-bit values.
typedef Array<3, int16_t> Array3DInt16;
///A 3D Array of unsigned 16-bit values.
typedef Array<3, uint16_t> Array3DUint16;
///A 3D Array of signed 32-bit values.
typedef Array<3, int32_t> Array3DInt32;
///A 3D Array of unsigned 32-bit values.
typedef Array<3, uint32_t> Array3DUint32;

}
