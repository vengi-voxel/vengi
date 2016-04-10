#pragma once

#include "PolyVox/LoggingImpl.h"
#include "core/Log.h"

namespace Cubiquity {

class Logger: public PolyVox::Logger {
public:
	Logger() :
			PolyVox::Logger() {
	}

	virtual ~Logger() {
	}

	void logTraceMessage(const std::string& message) override {
		Log::trace("%s", message.c_str());
	}

	void logDebugMessage(const std::string& message) override {
		Log::debug("%s", message.c_str());
	}

	void logInfoMessage(const std::string& message) override {
		Log::info("%s", message.c_str());
	}

	void logWarningMessage(const std::string& message) override {
		Log::warn("%s", message.c_str());
	}

	void logErrorMessage(const std::string& message) override {
		Log::error("%s", message.c_str());
	}

	void logFatalMessage(const std::string& message) override {
		Log::error("%s", message.c_str());
	}
};

}
