#pragma once

#include "Logging.h"

#include <sstream>
#include <string>

namespace PolyVox {

namespace Impl {

// Used for building the log messages - convert a list of variables into a string.
// Based on second approach here: http://stackoverflow.com/a/25386444/2337254
template<typename ... Args>
std::string argListToString(const Args& ... args) {
	return "";
}

// Log trace message
template<typename ... Args>
void logTraceMessage(Args const& ... messageArgs) {
	std::string message = argListToString(messageArgs...);
	getLoggerInstance()->logTraceMessage(message);
}

template<typename ... Args>
void logTraceMessageIf(bool condition, Args const& ... messageArgs) {
	if (condition) {
		logTraceMessage(messageArgs...);
	}
}

// Log debug message
template<typename ... Args>
void logDebugMessage(Args const& ... messageArgs) {
	std::string message = argListToString(messageArgs...);
	getLoggerInstance()->logDebugMessage(message);
}

template<typename ... Args>
void logDebugMessageIf(bool condition, Args const& ... messageArgs) {
	if (condition) {
		logDebugMessage(messageArgs...);
	}
}

// Log info message
template<typename ... Args>
void logInfoMessage(Args const& ... messageArgs) {
	std::string message = argListToString(messageArgs...);
	getLoggerInstance()->logInfoMessage(message);
}

template<typename ... Args>
void logInfoMessageIf(bool condition, Args const& ... messageArgs) {
	if (condition) {
		logInfoMessage(messageArgs...);
	}
}

// Log warning message
template<typename ... Args>
void logWarningMessage(Args const& ... messageArgs) {
	std::string message = argListToString(messageArgs...);
	getLoggerInstance()->logWarningMessage(message);
}

template<typename ... Args>
void logWarningMessageIf(bool condition, Args const& ... messageArgs) {
	if (condition) {
		logWarningMessage(messageArgs...);
	}
}

// Log error message
template<typename ... Args>
void logErrorMessage(Args const& ... messageArgs) {
	std::string message = argListToString(messageArgs...);
	getLoggerInstance()->logErrorMessage(message);
}

template<typename ... Args>
void logErrorMessageIf(bool condition, Args const& ... messageArgs) {
	if (condition) {
		logErrorMessage(messageArgs...);
	}
}

// Log fatal message
template<typename ... Args>
void logFatalMessage(Args const& ... messageArgs) {
	std::string message = argListToString(messageArgs...);
	getLoggerInstance()->logFatalMessage(message);
}

template<typename ... Args>
void logFatalMessageIf(bool condition, Args const& ... messageArgs) {
	if (condition) {
		logFatalMessage(messageArgs...);
	}
}

}

}

#define POLYVOX_LOG_TRACE(...) PolyVox::Impl::logTraceMessage(__VA_ARGS__)

#define POLYVOX_LOG_DEBUG(...) PolyVox::Impl::logDebugMessage(__VA_ARGS__)

#define POLYVOX_LOG_INFO(...) PolyVox::Impl::logInfoMessage(__VA_ARGS__)

#define POLYVOX_LOG_WARNING(...) PolyVox::Impl::logWarningMessage(__VA_ARGS__)
