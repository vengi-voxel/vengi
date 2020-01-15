/**
 * @file
 */

#pragma once

#include "voxelworld/WorldPager.h"

namespace client {

class ClientPager : public voxelworld::WorldPager {
public:
	ClientPager(const voxelformat::VolumeCachePtr& volumeCache);
};

typedef std::shared_ptr<ClientPager> ClientPagerPtr;

}
