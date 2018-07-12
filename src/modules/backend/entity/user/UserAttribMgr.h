/**
 * @file
 */

#pragma once

#include "persistence/ForwardDecl.h"
#include "backend/entity/EntityId.h"
#include "attrib/Attributes.h"
#include "core/IComponent.h"
#include "persistence/ISavable.h"
#include "collection/ConcurrentSet.h"
#include "AttribModel.h"
#include <vector>

namespace backend {

/**
 * @brief Manages the saving and loading of the current attribute values.
 *
 * @note All @c attrib::Container instances must already be applied. Otherwise
 * the loaded current values might get capped to their min/max value.
 */
class UserAttribMgr : public persistence::ISavable, public core::IComponent {
private:
	static constexpr uint32_t FOURCC = FourCC('A','T','T','R');
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

	bool init() override;
	void shutdown() override;

	bool getDirtyModels(Models& models) override;
};

}
