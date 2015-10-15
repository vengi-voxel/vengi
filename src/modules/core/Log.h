#pragma once

#include <cstdio>
#include <memory>

namespace core {
class Var;
typedef std::shared_ptr<Var> VarPtr;
}

class Log {
private:
	core::VarPtr _logLevel;

	Log();

	static inline Log& get() {
		static Log log;
		return log;
	}

	void vsnprint(const char* msg, va_list args);

public:
	static void trace(const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void debug(const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void info(const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void warn(const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void error(const char* msg, ...) __attribute__((format(printf, 1, 2)));
};
