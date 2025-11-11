/**
 * @file
 */

#include "palette/Palette.h"
#include "voxel/RawVolume.h"

namespace voxel {

class ClipboardData {
private:
	bool _disposeAfterUse = false;

public:
	voxel::RawVolume *volume = nullptr;
	palette::Palette *palette = nullptr;

	ClipboardData() = default;
	ClipboardData(const voxel::RawVolume *v, const palette::Palette *p, bool disposeAfterUse);
	ClipboardData(const voxel::RawVolume *v, const palette::Palette &p, bool disposeAfterUse);
	ClipboardData(voxel::RawVolume *v, const palette::Palette *p, bool disposeAfterUse);
	ClipboardData(voxel::RawVolume *v, const palette::Palette &p, bool disposeAfterUse);
	ClipboardData(const ClipboardData &v);
	ClipboardData(ClipboardData &&v);
	~ClipboardData();

	operator bool() const {
		return volume != nullptr && palette != nullptr;
	}

	bool dispose() const;

	ClipboardData &operator=(const ClipboardData &v);

	ClipboardData &operator=(ClipboardData &&v);
};

} // namespace voxel
