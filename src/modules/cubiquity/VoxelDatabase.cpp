#include "VoxelDatabase.h"
#include "PolyVox/Region.h"
#include "Logging.h"

namespace Cubiquity {

// See: http://stackoverflow.com/a/18529061
// Th actual bitwise logic differs from that given in 'Real Time Collision Detection', but
// matches that given here: http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
uint64_t Part1By2(uint64_t x) {
	x &= 0x1fffff;
	x = (x | x << 32) & 0x1f00000000ffff;
	x = (x | x << 16) & 0x1f0000ff0000ff;
	x = (x | x << 8) & 0x100f00f00f00f00f;
	x = (x | x << 4) & 0x10c30c30c30c30c3;
	x = (x | x << 2) & 0x1249249249249249;
	return x;
}

// See: http://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
uint64_t EncodeMorton3(uint64_t x, uint64_t y, uint64_t z) {
	return (Part1By2(z) << 2) + (Part1By2(y) << 1) + Part1By2(x);
}

// This function encodes a Region as a 64-bit integer so that it can be used as a key to access chunk data in the SQLite database.
// A region actually contains more than 64-bits of data so some has to be lost here. Specifically we assume that we already know
// the size of the region (so we only have to encode it's lower corner and not its upper corner or extents), and we also restrict
// the range of valid coordinates. A Region's coordinates are represented by 3-bits of data, but we only support converting to a key
// if every coordinate can be represented by 21 bits of data. This way we can fit three coordinates only 63 bits of data. This limits
// the range of values to +/- 2^20, which is enough for our purposes.
uint64_t regionToKey(const PolyVox::Region& region) {
	// Cast to unsigned values so that bit shifting works predictably.
	uint32_t x = static_cast<uint32_t>(region.getLowerX());
	uint32_t y = static_cast<uint32_t>(region.getLowerY());
	uint32_t z = static_cast<uint32_t>(region.getLowerZ());

	// The magnitude of our input values is fairly restricted, but the values could stil be negative. This means the sign bit could
	// be set and this needs to be encoded as well. We therefore perform a left rotate on the bits to bring the sign bit into the LSB.
	x = rotateLeft(x);
	y = rotateLeft(y);
	z = rotateLeft(z);

	// Now convert to 64-bits
	uint64_t x64 = x;
	uint64_t y64 = y;
	uint64_t z64 = z;

	// Morten-encode the components to give our final key
	uint64_t result = EncodeMorton3(x64, y64, z64);

	// Return the combined value
	return result;
}

}
