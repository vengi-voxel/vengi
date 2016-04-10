#pragma once

#include <limits>

namespace Cubiquity {

template<typename StorageType>
class BitField {
public:
	BitField(StorageType initialValue = 0) :
			_bits(initialValue) {
	}

	bool operator==(const BitField& rhs) const  {
		return _bits == rhs._bits;
	}

	bool operator!=(const BitField& rhs) const  {
		return !(*this == rhs);
	}

	StorageType getBits(size_t MSB, size_t LSB) const {
		const size_t noOfBitsToGet = (MSB - LSB) + 1;

		// Build a mask containing all '0's except for the least significant bits (which are '1's).
		StorageType mask = std::numeric_limits<StorageType>::max(); //Set to all '1's
		mask = mask << noOfBitsToGet; // Insert the required number of '0's for the lower bits
		mask = ~mask; // And invert

		// Move the desired bits into the LSBs and mask them off
		StorageType result = (_bits >> LSB) & mask;

		return result;
	}

	void setBits(size_t MSB, size_t LSB, StorageType bitsToSet) {
		const size_t noOfBitsToSet = (MSB - LSB) + 1;

		StorageType mask = std::numeric_limits<StorageType>::max(); //Set to all '1's
		mask = mask << noOfBitsToSet; // Insert the required number of '0's for the lower bits
		mask = ~mask; // And invert
		mask = mask << LSB;

		bitsToSet = (bitsToSet << LSB) & mask;

		_bits = (_bits & ~mask) | bitsToSet;
	}

	StorageType allBits() {
		return _bits;
	}

	void clearAllBits() {
		_bits = 0;
	}

private:
	StorageType _bits;
};

}
