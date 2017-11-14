/**
 * @file
 */

#include "persistence/DBHandler.h"
#include "backend/entity/EntityId.h"
#include "attrib/Attributes.h"

namespace backend {

/**
 * @brief Manages the saving and loading of the current attribute values.
 *
 * @note All @c attrib::Container instances must already be applied. Otherwise
 * the loaded current values might get capped to their min/max value.
 */
class UserAttribMgr {
private:
	EntityId _userId;
	attrib::Attributes& _attribs;
	persistence::DBHandlerPtr _dbHandler;
public:
	UserAttribMgr(EntityId userId,
			attrib::Attributes& attribs,
			const persistence::DBHandlerPtr& dbHandler);

	bool init();
	void shutdown();
};

}
