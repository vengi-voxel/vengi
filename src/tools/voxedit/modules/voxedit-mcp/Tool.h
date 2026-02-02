/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "network/ProtocolMessage.h"
#include "json/JSON.h"

namespace voxedit {

class SceneManager;

struct ToolContext {
	SceneManager *sceneMgr = nullptr;
	bool (*result)(const nlohmann::json &id, const core::String &text, bool isError);
};

class Tool {
protected:
	const core::String _name;
	nlohmann::json _tool;

	bool sendMessage(ToolContext &ctx, const network::ProtocolMessage &msg);
	bool sendCommand(ToolContext &ctx, const core::String &cmd, const nlohmann::json &id);
	const core::String &rconPassword() const;

	core::UUID argsUUID(const nlohmann::json &args) const;
	core::UUID argsParentUUID(const nlohmann::json &args) const;
	core::UUID argsReferenceUUID(const nlohmann::json &args) const;

	static nlohmann::json propUUID();
	static nlohmann::json propParentUUID();
	static nlohmann::json propReferenceUUID();
	static nlohmann::json propTypeDescription(const core::String &type, const core::String &description);

public:
	/**
	 * @brief Implementations of this class have to create their input schema in the ctor and store it in @c _tool
	 */
	Tool(const core::String &name) : _name(name) {
		_tool["name"] = _name.c_str();
	}

	virtual ~Tool() = default;

	inline const core::String &name() const {
		return _name;
	}

	/**
	 * @brief Returns the input schema for this tool - created in the ctor
	 */
	inline const nlohmann::json &inputSchema() const {
		return _tool;
	}

	/**
	 * @brief Enery end point of this function must call @c ctx.result()
	 */
	virtual bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) = 0;
};

} // namespace voxedit