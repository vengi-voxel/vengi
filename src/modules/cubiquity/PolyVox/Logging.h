#pragma once

#include <iostream>
#include <string>

// We expose the logger class to the user so that they can provide custom implementations
// to redirect PolyVox's log messages. However, it is not expected that user code will make 
// use of PolyVox's logging macros s these are part of the private implementation.
namespace PolyVox {

class Logger {
public:
	Logger() {
	}
	;
	virtual ~Logger() {
	}

	virtual void logTraceMessage(const std::string& message) = 0;
	virtual void logDebugMessage(const std::string& message) = 0;
	virtual void logInfoMessage(const std::string& message) = 0;
	virtual void logWarningMessage(const std::string& message) = 0;
	virtual void logErrorMessage(const std::string& message) = 0;
	virtual void logFatalMessage(const std::string& message) = 0;
};

class DefaultLogger: public Logger {
public:
	DefaultLogger() :
			Logger() {
	}

	virtual ~DefaultLogger() {
	}

	// Appending the 'std::endl' forces the stream to be flushed.
	void logTraceMessage(const std::string& /*message*/) {
	}

	void logDebugMessage(const std::string& /*message*/) {
	}

	void logInfoMessage(const std::string& message) {
		std::cout << message << std::endl;
	}

	void logWarningMessage(const std::string& message) {
		std::cerr << message << std::endl;
	}

	void logErrorMessage(const std::string& message) {
		std::cerr << message << std::endl;
	}

	void logFatalMessage(const std::string& message) {
		std::cerr << message << std::endl;
	}
};

inline Logger*& getLoggerInstance() {
	static Logger* s_pLogger = new DefaultLogger;
	return s_pLogger;
}

inline void setLoggerInstance(Logger* pLogger) {
	getLoggerInstance() = pLogger;
}

}
