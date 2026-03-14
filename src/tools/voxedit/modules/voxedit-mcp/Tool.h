/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/UUID.h"
#include "json/JSON.h"

namespace network {
class ProtocolMessage;
}

namespace voxedit {

class SceneManager;

struct ToolContext {
	SceneManager *sceneMgr = nullptr;
	bool (*result)(const json::Json &id, const core::String &text, bool isError);
};

class Tool {
protected:
	const core::String _name;
	json::Json _tool;

	bool sendMessage(ToolContext &ctx, const network::ProtocolMessage &msg);
	bool sendCommand(ToolContext &ctx, const core::String &cmd, const json::Json &id);
	const core::String &rconPassword() const;

	core::UUID argsUUID(const json::Json &args) const;
	core::UUID argsParentUUID(const json::Json &args) const;
	core::UUID argsReferenceUUID(const json::Json &args) const;

	static json::Json propVoxels();
	static json::Json propUUID();
	static json::Json propParentUUID();
	static json::Json propReferenceUUID();
	static json::Json propTypeDescription(const core::String &type, const core::String &description);

public:
	/**
	 * @brief Implementations of this class have to create their input schema in the ctor and store it in @c _tool
	 */
	Tool(const core::String &name);

	virtual ~Tool() = default;

	inline const core::String &name() const {
		return _name;
	}

	/**
	 * @brief Returns the input schema for this tool - created in the ctor
	 */
	inline const json::Json &inputSchema() const {
		return _tool;
	}

	/**
	 * @brief Enery end point of this function must call @c ctx.result()
	 */
	virtual bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) = 0;
};

} // namespace voxedit