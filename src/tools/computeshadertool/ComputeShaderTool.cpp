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
#include "util/IncludeUtil.h"
#include <stack>
#include <string>

ComputeShaderTool::ComputeShaderTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "computeshadertool");
}

ComputeShaderTool::~ComputeShaderTool() {
}

bool ComputeShaderTool::parse(const std::string& buffer) {
	return computeshadertool::parse(buffer, _computeFilename, _kernels, _structs, _constants);
}

core::AppState ComputeShaderTool::onConstruct() {
	registerArg("--shader").setShort("-s").setDescription("The base name of the shader to create the c++ bindings for").setMandatory();
	registerArg("--shadertemplate").setShort("-t").setDescription("The shader template file").setMandatory();
	registerArg("--namespace").setShort("-n").setDescription("Namespace to generate the source in").setDefaultValue("compute");
	registerArg("--shaderdir").setShort("-d").setDescription("Directory to load the shader from").setDefaultValue("shaders/");
	registerArg("--sourcedir").setDescription("Directory to generate the source in").setMandatory();
	registerArg("-I").setDescription("Add additional include dir");
	return Super::onConstruct();
}

std::string ComputeShaderTool::getSource(const std::string& file) const {
	const io::FilesystemPtr& fs = filesystem();
	std::string src = fs->load(file);

	src = util::handleIncludes(src, _includeDirs);
	int level = 0;
	while (core::string::contains(src, "#include")) {
		src = util::handleIncludes(src, _includeDirs);
		++level;
		if (level >= 10) {
			Log::warn("Abort shader include loop for %s", file.c_str());
			break;
		}
	}
	return src;
}

core::AppState ComputeShaderTool::onRunning() {
	const std::string shaderfile          = getArgVal("--shader");
	_shaderTemplateFile                   = getArgVal("--shadertemplate");
	_namespaceSrc                         = getArgVal("--namespace");
	_shaderDirectory                      = getArgVal("--shaderdir");
	_sourceDirectory                      = getArgVal("--sourcedir",
			_filesystem->basePath() + "src/modules/" + _namespaceSrc + "/");
	_postfix                              = getArgVal("--postfix", "");

	// handle include dirs
	_includeDirs.push_back(".");
	int index = 0;
	for (;;) {
		const std::string& dir = getArgVal("-I", "", &index);
		if (dir.empty()) {
			break;
		}
		_includeDirs.push_back(dir);
	}

	if (!core::string::endsWith(_shaderDirectory, "/")) {
		_shaderDirectory = _shaderDirectory + "/";
	}
	Log::debug("Using %s as output directory", _sourceDirectory.c_str());
	Log::debug("Using %s as namespace", _namespaceSrc.c_str());
	Log::debug("Using %s as shader directory", _shaderDirectory.c_str());

	Log::debug("Preparing shader file %s", shaderfile.c_str());
	_computeFilename = shaderfile + COMPUTE_POSTFIX;
	const bool changedDir = filesystem()->pushDir(std::string(core::string::extractPath(shaderfile.c_str())));
	const std::string computeBuffer = getSource(_computeFilename);
	if (computeBuffer.empty()) {
		Log::error("Could not load %s", _computeFilename.c_str());
		_exitCode = 127;
		return core::AppState::Cleanup;
	}

	compute::Shader shader;
	const std::string& computeSrcSource = shader.getSource(computeBuffer, false);

	_name = std::string(core::string::extractFilename(shaderfile.c_str()));
	if (!parse(computeSrcSource)) {
		_exitCode = 1;
		return core::AppState::Cleanup;
	}
	const std::string& templateShader = filesystem()->load(_shaderTemplateFile);
	if (!computeshadertool::generateSrc(filesystem(), templateShader, _name, _namespaceSrc, _shaderDirectory, _sourceDirectory, _kernels, _structs, _constants, _postfix, computeBuffer)) {
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

CONSOLE_APP(ComputeShaderTool)
