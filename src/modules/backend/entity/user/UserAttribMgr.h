/**
 * @file
 */

#include "persistence/DBHandler.h"
#include "backend/entity/EntityId.h"
#include "attrib/Attributes.h"
#include "persistence/ISavable.h"
#include "persistence/PersistenceMgr.h"
#include "collection/ConcurrentSet.h"
#include "AttribModel.h"
#include <vector>
#include <unordered_set>

namespace backend {

/**
 * @brief Manages the saving and loading of the current attribute values.
 *
 * @note All @c attrib::Container instances must already be applied. Otherwise
 * the loaded current values might get capped to their min/max value.
 */
class UserAttribMgr : public persistence::ISavable {
private:
	EntityId _userId;
	attrib::Attributes& _attribs;
	using Collection = collection::ConcurrentSet<attrib::DirtyValue>;
	Collection _dirtyAttributeTypes;
	persistence::DBHandlerPtr _dbHandler;
	persistence::PersistenceMgrPtr _persistenceMgr;
	std::vector<db::AttribModel> _dirtyModels;

	void onAttribChange(const attrib::DirtyValue& v);
public:
	UserAttribMgr(EntityId userId,
			attrib::Attributes& attribs,
			const persistence::DBHandlerPtr& dbHandler,
			const persistence::PersistenceMgrPtr& persistenceMgr);

	bool init();
	void shutdown();

	bool getDirtyModels(Models& models) override;
};

}
