/**
 * @file
 * @ingroup LUA
 */
#pragma once

#include "TreeNode.h"
#include "commonlua/LUA.h"

namespace backend {

/**
 * @see @ai{LUAAIRegistry}
 */
class LUATreeNode : public TreeNode {
protected:
	lua_State* _s;

	ai::TreeNodeStatus runLUA(const AIPtr& entity, int64_t deltaMillis);

public:
	class LUATreeNodeFactory : public ITreeNodeFactory {
	private:
		lua_State* _s;
		core::String _type;
	public:
		LUATreeNodeFactory(lua_State* s, const core::String& typeStr);

		inline const core::String& type() const {
			return _type;
		}

		TreeNodePtr create(const TreeNodeFactoryContext* ctx) const override;
	};

	LUATreeNode(const core::String& name, const core::String& parameters, const ConditionPtr& condition, lua_State* s, const core::String& type);
	~LUATreeNode();

	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override;
};

}
