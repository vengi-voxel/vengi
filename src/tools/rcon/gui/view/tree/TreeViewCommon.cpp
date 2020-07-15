/**
 * @file
 */
#include "TreeViewCommon.h"
#include "ai-shared/common/TreeNodeStatus.h"

namespace ai {
namespace debug {

#define E(x) #x
const char *stateNames[] = {
E(UNKNOWN),
E(CANNOTEXECUTE),
E(RUNNING),
E(FINISHED),
E(FAILED),
E(EXCEPTION)
};
#undef E
static_assert(sizeof(stateNames) / sizeof(*stateNames) == MAX_TREENODESTATUS, "State names don't match");

}
}
