/**
 * @file
 */

#include "ShaderTool.h"
#include "sauce/ShaderToolInjector.h"
#include "core/App.h"
#include "core/Process.h"
#include "video/Shader.h"

ShaderTool::ShaderTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		core::App(filesystem, eventBus, 0) {
	init("engine", "shadertool");
}

ShaderTool::~ShaderTool() {
}

core::AppState ShaderTool::onRunning() {
	if (_argc != 3) {
		_exitCode = 1;
		Log::error("Usage: %s <path/to/glslangvalidator> <shaderfile>", _argv[0]);
		return core::AppState::Cleanup;
	}

	const std::string glslangValidatorBin = _argv[1];
	Log::debug("Using glslangvalidator binary: %s", glslangValidatorBin.c_str());

	const std::string shaderfile = _argv[2];

	Log::debug("Preparing shader file %s", shaderfile.c_str());
	const std::string fragmentFilename = shaderfile + FRAGMENT_POSTFIX;
	const std::string fragmentBuffer = filesystem()->load(fragmentFilename);
	if (fragmentBuffer.empty()) {
		Log::error("Could not load %s", fragmentFilename.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	const std::string vertexFilename = shaderfile + VERTEX_POSTFIX;
	const std::string vertexBuffer = filesystem()->load(vertexFilename);
	if (vertexBuffer.empty()) {
		Log::error("Could not load %s", vertexFilename.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	video::Shader shader;
	const std::string& fragmentSource = shader.getSource(video::ShaderType::SHADER_FRAGMENT, fragmentBuffer);
	const std::string& vertexSource = shader.getSource(video::ShaderType::SHADER_VERTEX, vertexBuffer);

	Log::debug("Writing shader file %s to %s", shaderfile.c_str(), filesystem()->homePath().c_str());
	std::string finalFragmentFilename = _appname + "-" + fragmentFilename;
	std::string finalVertexFilename = _appname + "-" + vertexFilename;
	filesystem()->write(finalFragmentFilename, fragmentSource);
	filesystem()->write(finalVertexFilename, vertexSource);

	Log::debug("Validating shader file %s", shaderfile.c_str());

	std::vector<std::string> fragmentArgs;
	fragmentArgs.push_back(filesystem()->homePath() + finalFragmentFilename);
	int fragmentValidationExitCode = core::Process::exec(glslangValidatorBin, fragmentArgs);

	std::vector<std::string> vertexArgs;
	vertexArgs.push_back(filesystem()->homePath() + finalVertexFilename);
	int vertexValidationExitCode = core::Process::exec(glslangValidatorBin, vertexArgs);

	if (fragmentValidationExitCode != 0) {
		_exitCode = fragmentValidationExitCode;
	} else if (vertexValidationExitCode != 0) {
		_exitCode = vertexValidationExitCode;
	}

	return core::AppState::Cleanup;
}

int main(int argc, char *argv[]) {
	return getInjector()->get<ShaderTool>()->startMainLoop(argc, argv);
}
