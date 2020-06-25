/**
 * @file
 */

#pragma once

#include "core/EventBus.h"
#include "TrazeTypes.h"
#include "voxel/RawVolume.h"
#include "core/SharedPtr.h"
#include <vector>

namespace traze {

EVENTBUSPAYLOADEVENT(NewGridEvent, core::SharedPtr<voxel::RawVolume>);
EVENTBUSPAYLOADEVENT(NewGamesEvent, std::vector<GameInfo>);
EVENTBUSPAYLOADEVENT(PlayerListEvent, std::vector<Player>);
EVENTBUSPAYLOADEVENT(TickerEvent, Ticker);
EVENTBUSPAYLOADEVENT(SpawnEvent, Spawn);
EVENTBUSPAYLOADEVENT(BikeEvent, Bike);
EVENTBUSPAYLOADEVENT(ScoreEvent, Score);

}
