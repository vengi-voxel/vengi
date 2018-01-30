/**
 * @file
 */

#include "ComputeShaderTool.h"
#include "core/Assert.h"
#include "core/String.h"
#include "io/Filesystem.h"
#include "compute/Shader.h"
#include "Generator.h"
#include "Parser.h"
#include "Util.h"
#include <stack>
#include <string>

ComputeShaderTool::ComputeShaderTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "computeshadertool");
}

ComputeShaderTool::~ComputeShaderTool() {
}

bool ComputeShaderTool::parse(const std::string& buffer) {
	return computeshadertool::parse(buffer, _computeFilename, _kernels, _structs);
}

core::AppState ComputeShaderTool::onConstruct() {
	registerArg("--shader").setShort("-s").setDescription("The base name of the shader to create the c++ bindings for").setMandatory();
	registerArg("--shadertemplate").setShort("-t").setDescription("The shader template file").setMandatory();
	registerArg("--namespace").setShort("-n").setDescription("Namespace to generate the source in").setDefaultValue("compute");
	registerArg("--shaderdir").setShort("-d").setDescription("Directory to load the shader from").setDefaultValue("shaders/");
	registerArg("--sourcedir").setDescription("Directory to generate the source in").setMandatory();
	return Super::onConstruct();
}

core::AppState ComputeShaderTool::onRunning() {
	const std::string shaderfile          = getArgVal("--shader");
	_shaderTemplateFile                   = getArgVal("--shadertemplate");
	_namespaceSrc                         = getArgVal("--namespace");
	_shaderDirectory                      = getArgVal("--shaderdir");
	_sourceDirectory                      = getArgVal("--sourcedir",
			_filesystem->basePath() + "src/modules/" + _namespaceSrc + "/");

	if (!core::string::endsWith(_shaderDirectory, "/")) {
		_shaderDirectory = _shaderDirectory + "/";
	}
	Log::debug("Using %s as output directory", _sourceDirectory.c_str());
	Log::debug("Using %s as namespace", _namespaceSrc.c_str());
	Log::debug("Using %s as shader directory", _shaderDirectory.c_str());

	Log::debug("Preparing shader file %s", shaderfile.c_str());
	_computeFilename = shaderfile + COMPUTE_POSTFIX;
	const bool changedDir = filesystem()->pushDir(std::string(core::string::extractPath(shaderfile.c_str())));
	const std::string computeBuffer = filesystem()->load(_computeFilename);
	if (computeBuffer.empty()) {
		Log::error("Could not load %s", _computeFilename.c_str());
		return core::AppState::InitFailure;
	}

	compute::Shader shader;
	const std::string& computeSrcSource = shader.getSource(computeBuffer, false);

	_name = std::string(core::string::extractFilename(shaderfile.c_str()));
	if (!parse(computeSrcSource)) {
		return core::AppState::InitFailure;
	}
	const std::string& templateShader = filesystem()->load(_shaderTemplateFile);
	if (!computeshadertool::generateSrc(filesystem(), templateShader, _name, _namespaceSrc, _shaderDirectory, _sourceDirectory, _kernels, _structs)) {
		_exitCode = 100;
		return core::AppState::Cleanup;
	}

	const std::string& computeSource = shader.getSource(computeBuffer, true);

	if (changedDir) {
		filesystem()->popDir();
	}

	Log::debug("Writing shader file %s to %s", shaderfile.c_str(), filesystem()->homePath().c_str());
	std::string finalComputeFilename = _appname + "-" + _computeFilename;
	filesystem()->write(finalComputeFilename, computeSource);

	return core::AppState::Cleanup;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	ComputeShaderTool app(metric, filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
