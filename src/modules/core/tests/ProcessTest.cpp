/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Process.h"
#include "io/BufferedReadWriteStream.h"
#include "core/String.h"

namespace core {

TEST(ProcessTest, testExecEcho) {
#if defined(_WIN32) || defined(__CYGWIN__)
	// Test executing echo command on Windows via cmd.exe
	io::BufferedReadWriteStream stream;
	core::DynamicArray<core::String> args;
	args.push_back("/c");
	args.push_back("echo Hello World");

	int result = Process::exec("cmd", args, nullptr, &stream);

	// Echo should succeed
	EXPECT_EQ(0, result);

	// Check that we got some output
	EXPECT_GT(stream.size(), 0);

	// Convert output to string and check it contains our text
	core::String output((const char*)stream.getBuffer(), (int)stream.size());
	EXPECT_TRUE(output.contains("Hello World"));
#elif defined(__linux__) || defined(__APPLE__)
	// Test executing echo command on Unix-like systems
	io::BufferedReadWriteStream stream;
	core::DynamicArray<core::String> args;
	args.push_back("Hello World");

	int result = Process::exec("/bin/echo", args, nullptr, &stream);

	// Echo should succeed
	EXPECT_EQ(0, result);

	// Check that we got some output
	EXPECT_GT(stream.size(), 0);

	// Convert output to string and check it contains our text
	core::String output((const char*)stream.getBuffer(), (int)stream.size());
	EXPECT_TRUE(output.contains("Hello World"));
#endif
}

TEST(ProcessTest, testExecWithoutOutput) {
#if defined(_WIN32) || defined(__CYGWIN__)
	// Test executing echo command without capturing output
	core::DynamicArray<core::String> args;
	args.push_back("/c");
	args.push_back("echo Hello World");

	int result = Process::exec("cmd", args, nullptr, nullptr);

	// Should succeed even without output capture
	EXPECT_EQ(0, result);
#elif defined(__linux__) || defined(__APPLE__)
	// Test executing echo command without capturing output
	core::DynamicArray<core::String> args;
	args.push_back("Hello World");

	int result = Process::exec("/bin/echo", args, nullptr, nullptr);

	// Should succeed even without output capture
	EXPECT_EQ(0, result);
#endif
}

TEST(ProcessTest, testExecWithMultipleArgs) {
#if defined(_WIN32) || defined(__CYGWIN__)
	// Test executing echo with multiple arguments on Windows
	io::BufferedReadWriteStream stream;
	core::DynamicArray<core::String> args;
	args.push_back("/c");
	args.push_back("echo Hello Multiple Arguments");

	int result = Process::exec("cmd", args, nullptr, &stream);

	EXPECT_EQ(0, result);
	EXPECT_GT(stream.size(), 0);

	core::String output((const char*)stream.getBuffer(), (int)stream.size());
	EXPECT_TRUE(output.contains("Hello"));
	EXPECT_TRUE(output.contains("Multiple"));
	EXPECT_TRUE(output.contains("Arguments"));
#elif defined(__linux__) || defined(__APPLE__)
	// Test executing echo with multiple arguments on Unix-like systems
	io::BufferedReadWriteStream stream;
	core::DynamicArray<core::String> args;
	args.push_back("Hello");
	args.push_back("Multiple");
	args.push_back("Arguments");

	int result = Process::exec("/bin/echo", args, nullptr, &stream);

	EXPECT_EQ(0, result);
	EXPECT_GT(stream.size(), 0);

	core::String output((const char*)stream.getBuffer(), (int)stream.size());
	EXPECT_TRUE(output.contains("Hello"));
	EXPECT_TRUE(output.contains("Multiple"));
	EXPECT_TRUE(output.contains("Arguments"));
#endif
}

TEST(ProcessTest, testExecInvalidCommand) {
	// Test executing a command that doesn't exist
	io::BufferedReadWriteStream stream;
	core::DynamicArray<core::String> args;

	int result = Process::exec("nonexistent_command_12345", args, nullptr, &stream);

	// Should fail (non-zero return code)
	EXPECT_NE(0, result);
}

TEST(ProcessTest, testExecEmptyArgs) {
#if defined(_WIN32) || defined(__CYGWIN__)
	// Test executing echo with minimal arguments
	io::BufferedReadWriteStream stream;
	core::DynamicArray<core::String> args;
	args.push_back("/c");
	args.push_back("echo.");

	int result = Process::exec("cmd", args, nullptr, &stream);

	// Should succeed
	EXPECT_EQ(0, result);
#elif defined(__linux__) || defined(__APPLE__)
	// Test executing echo with no arguments
	io::BufferedReadWriteStream stream;
	core::DynamicArray<core::String> args; // empty args

	int result = Process::exec("/bin/echo", args, nullptr, &stream);

	// Should succeed
	EXPECT_EQ(0, result);
#endif
}

TEST(ProcessTest, testExecWithWorkingDirectory) {
#if defined(_WIN32) || defined(__CYGWIN__)
	// Test executing a command with a specific working directory (Windows)
	io::BufferedReadWriteStream stream;
	core::DynamicArray<core::String> args;

	// Use 'cd' to show current directory
	int result = Process::exec("cmd", {"/c", "cd"}, "C:\\", &stream);

	EXPECT_EQ(0, result);
	EXPECT_GT(stream.size(), 0);

	core::String output((const char*)stream.getBuffer(), (int)stream.size());
	// Should show C:\ as the current directory
	EXPECT_TRUE(output.contains("C:"));
#elif defined(__linux__) || defined(__APPLE__)
	// Test executing a command with a specific working directory (Unix)
	io::BufferedReadWriteStream stream;
	core::DynamicArray<core::String> args;

	// Use 'pwd' to show current directory
	int result = Process::exec("/bin/pwd", args, "/tmp", &stream);

	EXPECT_EQ(0, result);
	EXPECT_GT(stream.size(), 0);

	core::String output((const char*)stream.getBuffer(), (int)stream.size());
	// Should show /tmp as the current directory
	EXPECT_TRUE(output.contains("/tmp"));
#endif
}

} // namespace core