#include "TreeViewCommon.h"

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
COMPILE_TIME_ASSERT(ARRAY_LENGTH(stateNames) == MAX_TREENODESTATUS);

}
}
