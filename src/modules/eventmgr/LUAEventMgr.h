
#include "commonlua/LUA.h"

namespace eventmgr {

class EventMgr;

extern void luaeventmgr_setup(lua_State* s, EventMgr* mgr);

}
