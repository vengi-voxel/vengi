/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/String.h"

namespace voxel {
class Region;
class RawVolumeWrapper;
class Voxel;
}

namespace voxelgenerator {

class LUAGenerator : public core::IComponent {
public:
	bool init() override;
	void shutdown() override;

	bool exec(const core::String& luaScript, voxel::RawVolumeWrapper* volume, const voxel::Region& region, const voxel::Voxel& voxel);
};

}
