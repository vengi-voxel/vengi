/**
 * @file
 */

#pragma once

#include "core/EventBus.h"
#include "TrazeTypes.h"
#include "voxel/polyvox/RawVolume.h"
#include <vector>

namespace traze {

EVENTBUSPAYLOADEVENT(NewGridEvent, voxel::RawVolume*);
EVENTBUSPAYLOADEVENT(NewGamesEvent, std::vector<GameInfo>);
EVENTBUSPAYLOADEVENT(PlayerListEvent, std::vector<Player>);
EVENTBUSPAYLOADEVENT(TickerEvent, Ticker);
EVENTBUSPAYLOADEVENT(SpawnEvent, Spawn);
EVENTBUSPAYLOADEVENT(BikeEvent, Bike);

}
