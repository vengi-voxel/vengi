/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "json/JSON.h"

namespace voxedit {

// JSON-RPC 2.0
static constexpr int MCP_PARSE_ERROR = -32700;
static constexpr int MCP_INVALID_REQUEST = -32600;
static constexpr int MCP_METHOD_NOT_FOUND = -32601;
static constexpr int MCP_INVALID_PARAMS = -32602;
static constexpr int MCP_INTERNAL_ERROR = -32603;
// Implementation-defined (legacy range). Used when the VoxEdit connection fails.
static constexpr int MCP_INIT_FAILED = -32000;
// MCP 2026-07-28 reserved codes
static constexpr int MCP_UNSUPPORTED_PROTOCOL_VERSION = -32022;

static const char *const McpMetaProtocolVersion = "io.modelcontextprotocol/protocolVersion";
static const char *const McpMetaClientInfo = "io.modelcontextprotocol/clientInfo";
static const char *const McpMetaClientCapabilities = "io.modelcontextprotocol/clientCapabilities";
static const char *const McpMetaServerInfo = "io.modelcontextprotocol/serverInfo";

const char *mcpLatestProtocolVersion();
const char *mcpLatestLegacyProtocolVersion();
const char *mcpDefaultProtocolVersion();
const char *mcpServerInstructions();

bool mcpIsSupportedProtocolVersion(const core::String &version);
bool mcpIsModernProtocolVersion(const core::String &version);
/**
 * @brief Legacy initialize negotiation: return @p asked when supported, otherwise the
 * latest legacy version. Empty @p asked also selects the latest legacy version.
 */
const char *mcpNegotiateProtocolVersion(const core::String &asked);

json::Json mcpSupportedProtocolVersions();
json::Json mcpServerCapabilities();
json::Json mcpServerInfo(const core::String &name, const core::String &version);
json::Json mcpRequestParams(const json::Json &request);
json::Json mcpRequestMeta(const json::Json &request);
bool mcpRequestHasModernMeta(const json::Json &request);
bool mcpRequestHasClientCapabilities(const json::Json &request);
core::String mcpRequestProtocolVersion(const json::Json &request);
json::Json mcpRequestClientInfo(const json::Json &request);

void mcpApplyCompleteResult(json::Json &result, const core::String &serverName, const core::String &serverVersion);
void mcpApplyCacheableResult(json::Json &result, int ttlMs, const char *cacheScope);
json::Json mcpDiscoverResult(const core::String &serverName, const core::String &serverVersion);
json::Json mcpInitializeResult(const char *protocolVersion, const core::String &serverName,
							   const core::String &serverVersion);
json::Json mcpUnsupportedProtocolVersionData(const core::String &requested);

} // namespace voxedit
