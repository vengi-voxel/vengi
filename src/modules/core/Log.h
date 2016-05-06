/**
 * @file
 */

#pragma once

#include <cstdio>
#include <memory>

class Log {
public:
	static void init();
	static void trace(const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void debug(const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void info(const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void warn(const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void error(const char* msg, ...) __attribute__((format(printf, 1, 2)));
};
