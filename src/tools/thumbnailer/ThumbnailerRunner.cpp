/**
 * @file
 */

#include "ThumbnailerRunner.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_process.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#if defined(_WIN32) || defined(__CYGWIN__)
#include <direct.h>
#else
#include <unistd.h>
#endif

namespace thumbnailer {

namespace {

#if defined(_WIN32) || defined(__CYGWIN__)
static const char *executableSuffix() {
	return ".exe";
}
#else
static const char *executableSuffix() {
	return "";
}
#endif

static core::String thumbnailerBinaryName() {
	return core::String::format("vengi-thumbnailer%s", executableSuffix());
}

static bool isExecutableFile(const core::String &path) {
	if (path.empty() || !io::Filesystem::sysExists(path)) {
		return false;
	}
	return true;
}

static void setError(core::String *errorMessage, const core::String &message) {
	Log::error("%s", message.c_str());
	if (errorMessage) {
		*errorMessage = message;
	}
}

} // namespace

core::String findThumbnailerExecutable(const core::String &modulePath) {
	const core::String binary = thumbnailerBinaryName();
	const core::String moduleDir = core::string::extractDir(modulePath);

	core::DynamicArray<core::String> candidates;
	candidates.reserve(6);
	// Same directory as the plugin / extension binary
	candidates.push_back(core::string::path(moduleDir, binary));
	// Windows layout: thumbnailer/vengi-thumbnailer next to the DLL
	candidates.push_back(core::string::path(moduleDir, "thumbnailer", binary));
	// macOS Quick Look appex:
	//   App.app/Contents/PlugIns/foo.appex/Contents/MacOS/foo
	// -> App.app/Contents/MacOS/vengi-thumbnailer
	candidates.push_back(core::string::path(moduleDir, "..", "..", "..", "..", "MacOS", binary));
	candidates.push_back(core::string::path(moduleDir, "..", "..", "..", "MacOS", binary));
	// Helper next to a non-bundled executable during local builds
	candidates.push_back(core::string::path(moduleDir, "..", "thumbnailer", binary));

	for (const core::String &candidate : candidates) {
		const core::String normalized = core::string::sanitizePath(candidate);
		if (isExecutableFile(normalized)) {
			return normalized;
		}
	}
	return core::String::Empty;
}

bool renderThumbnail(const RenderRequest &request, core::String *errorMessage) {
	if (request.inputFile.empty() || request.outputFile.empty()) {
		setError(errorMessage, "Thumbnailer input or output path is empty");
		return false;
	}
	if (request.size <= 0) {
		setError(errorMessage, "Thumbnailer size must be positive");
		return false;
	}

	core::String thumbnailerPath = request.thumbnailerPath;
	if (thumbnailerPath.empty()) {
		thumbnailerPath = findThumbnailerExecutable(request.modulePath);
	}
	if (thumbnailerPath.empty()) {
		setError(errorMessage, "Could not find vengi-thumbnailer executable");
		return false;
	}
	if (!isExecutableFile(thumbnailerPath)) {
		setError(errorMessage, core::String::format("Thumbnailer executable not found: %s", thumbnailerPath.c_str()));
		return false;
	}

	core::DynamicArray<core::String> arguments;
	arguments.reserve(8);
	if (request.useSceneCamera) {
		arguments.push_back("--use-scene-camera");
	}
	arguments.push_back("--size");
	arguments.push_back(core::String::format("%i", request.size));
	arguments.push_back("--input");
	arguments.push_back(request.inputFile);
	arguments.push_back("--output");
	arguments.push_back(request.outputFile);

	core::DynamicArray<const char *> argv;
	argv.reserve(arguments.size() + 2);
	argv.push_back(thumbnailerPath.c_str());
	for (const core::String &arg : arguments) {
		argv.push_back(arg.c_str());
	}
	argv.push_back(nullptr);

	const SDL_PropertiesID props = SDL_CreateProperties();
	if (props == 0) {
		setError(errorMessage, core::String::format("Failed to create process properties: %s", SDL_GetError()));
		return false;
	}

	SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void *)argv.data());

#if !defined(SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING)
	char *oldWorkingDirectory = nullptr;
	bool restoreWorkingDirectory = false;
#endif
	if (!request.workingDirectory.empty()) {
#ifdef SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING
		SDL_SetStringProperty(props, SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING, request.workingDirectory.c_str());
#else
		oldWorkingDirectory = SDL_GetCurrentDirectory();
		if (oldWorkingDirectory != nullptr) {
#if defined(_WIN32) || defined(__CYGWIN__)
			if (_chdir(request.workingDirectory.c_str()) == 0) {
#else
			if (chdir(request.workingDirectory.c_str()) == 0) {
#endif
				restoreWorkingDirectory = true;
			}
		}
#endif
	}

	SDL_Environment *environment = SDL_CreateEnvironment(true);
	if (environment) {
		// Isolate renderer state from the user's interactive configuration / crash UI.
		core::String isolatedHome = request.workingDirectory;
		if (isolatedHome.empty()) {
			isolatedHome = core::string::extractDir(request.outputFile);
		}
		if (!isolatedHome.empty()) {
			isolatedHome = core::string::sanitizeDirPath(isolatedHome);
			SDL_SetEnvironmentVariable(environment, "APP_HOMEPATH", isolatedHome.c_str(), true);
		}
		SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ENVIRONMENT_POINTER, environment);
	}

	SDL_Process *process = SDL_CreateProcessWithProperties(props);
	SDL_DestroyProperties(props);
	if (environment) {
		SDL_DestroyEnvironment(environment);
	}
#if !defined(SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING)
	if (restoreWorkingDirectory && oldWorkingDirectory != nullptr) {
#if defined(_WIN32) || defined(__CYGWIN__)
		_chdir(oldWorkingDirectory);
#else
		chdir(oldWorkingDirectory);
#endif
	}
	SDL_free(oldWorkingDirectory);
#endif
	if (process == nullptr) {
		setError(errorMessage, core::String::format("Failed to start %s: %s", thumbnailerPath.c_str(), SDL_GetError()));
		return false;
	}

	const int timeoutMs = request.timeoutMs > 0 ? request.timeoutMs : 30000;
	const Uint64 deadline = SDL_GetTicks() + (Uint64)timeoutMs;
	int exitCode = -1;
	bool finished = false;
	while (!(finished = SDL_WaitProcess(process, false, &exitCode))) {
		if (SDL_GetTicks() >= deadline) {
			SDL_KillProcess(process, true);
			SDL_WaitProcess(process, true, &exitCode);
			SDL_DestroyProcess(process);
			setError(errorMessage, core::String::format("Thumbnailer timed out after %i ms", timeoutMs));
			return false;
		}
		SDL_Delay(20);
	}
	SDL_DestroyProcess(process);

	if (!finished || exitCode != 0) {
		setError(errorMessage, core::String::format("Thumbnailer exited with code %i", exitCode));
		return false;
	}
	if (!io::Filesystem::sysExists(request.outputFile)) {
		setError(errorMessage, core::String::format("Thumbnailer did not create %s", request.outputFile.c_str()));
		return false;
	}
	return true;
}

} // namespace thumbnailer
