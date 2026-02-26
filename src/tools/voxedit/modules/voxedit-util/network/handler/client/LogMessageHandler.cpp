/**
 * @file
 */

#include "LogMessageHandler.h"
#include "core/Log.h"

namespace voxedit {

void LogMessageHandler::execute(const network::ClientId &, LogMessage *message) {
	const char *msg = message->message().c_str();
	switch (message->level()) {
	case Log::Level::Trace:
		Log::trace("server: %s", msg);
		break;
	case Log::Level::Debug:
		Log::debug("server: %s", msg);
		break;
	case Log::Level::Info:
		Log::info("server: %s", msg);
		break;
	case Log::Level::Warn:
		Log::warn("server: %s", msg);
		break;
	case Log::Level::Error:
		Log::error("server: %s", msg);
		break;
	default:
		Log::info("server: %s", msg);
		break;
	}
}

} // namespace voxedit
