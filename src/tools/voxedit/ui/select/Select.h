#pragma once

#include "voxel/polyvox/RawVolume.h"

namespace selections {

#define SelectionSingleton(Class) static Class& get() { static Class instance; return instance; }

class Select {
public:
	virtual ~Select() {
	}

	virtual bool execute(const voxel::RawVolume *model, voxel::RawVolume *selection, const glm::ivec3& pos) const = 0;
};

}
