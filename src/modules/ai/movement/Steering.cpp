#include "Steering.h"
#include "SelectionFlee.h"
#include "SelectionSeek.h"
#include "GroupFlee.h"
#include "GroupSeek.h"
#include "TargetFlee.h"
#include "TargetSeek.h"
#include "Wander.h"
#include "WeightedSteering.h"

namespace ai {
namespace movement {

STEERING_FACTORY_IMPL(TargetSeek)
STEERING_FACTORY_IMPL(TargetFlee)
STEERING_FACTORY_IMPL(GroupSeek)
STEERING_FACTORY_IMPL(GroupFlee)
STEERING_FACTORY_IMPL(Wander)
STEERING_FACTORY_IMPL(SelectionSeek)
STEERING_FACTORY_IMPL(SelectionFlee)

}
}
