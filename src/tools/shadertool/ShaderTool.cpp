/**
 * @file
 */

#include "ShaderTool.h"
#include "core/io/Filesystem.h"
#include "core/Process.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/Assert.h"
#include "core/GameConfig.h"
#include "util/IncludeUtil.h"
#include "video/Shader.h"
#include "Generator.h"
#include "Parser.h"

ShaderTool::ShaderTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "shadertool");
	_initialLogLevel = SDL_LOG_PRIORITY_WARN;
}

bool ShaderTool::parse(const core::String& buffer, bool vertex) {
	return shadertool::parse(_shaderStruct, _shaderfile, buffer, vertex);
}

core::AppState ShaderTool::onConstruct() {
	registerArg("--glslang").setShort("-g").setDescription("Path to glslangvalidator binary").setMandatory();
	registerArg("--shader").setShort("-s").setDescription("The base name of the shader to create the c++ bindings for").setMandatory();
	registerArg("--shadertemplate").setShort("-t").setDescription("The shader template file").setMandatory();
	registerArg("--buffertemplate").setShort("-b").setDescription("The uniform buffer template file").setMandatory();
	registerArg("--namespace").setShort("-n").setDescription("Namespace to generate the source in").setDefaultValue("shader");
	registerArg("--shaderdir").setShort("-d").setDescription("Directory to load the shader from").setDefaultValue("shaders/");
	registerArg("--sourcedir").setDescription("Directory to generate the source in").setMandatory();
	registerArg("-I").setDescription("Add additional include dir");
	registerArg("--printincludes").setDescription("Print the includes for the given shader");
	Log::trace("Set some shader config vars to let the validation work");
	core::Var::get(cfg::ClientGamma, "2.2", core::CV_SHADER);
	core::Var::get(cfg::ClientFog, "true", core::CV_SHADER);
	core::Var::get(cfg::ClientShadowMap, "true", core::CV_SHADER);
	core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER);
	core::Var::get(cfg::ClientDebugShadowMapCascade, "false", core::CV_SHADER);
	return Super::onConstruct();
}

void ShaderTool::validate(const core::String& name) {
	const core::String& writePath = filesystem()->homePath();
	std::vector<std::string> args;
	args.push_back(writePath + name);
	Log::debug("Execute glslang validator with the following commandline: %s %s", _glslangValidatorBin.c_str(),
			   args[0].c_str());
	char output[4096] = "";
	int exitCode = core::Process::exec(_glslangValidatorBin, args, nullptr, sizeof(output), output);
	if (exitCode != 0) {
		Log::error("Failed to validate shader '%s'. Exitcode: %i", name.c_str(), exitCode);
		if (output[0] != '\0') {
			Log::error("%s", output);
		}
		Log::debug("%s %s%s", _glslangValidatorBin.c_str(), writePath.c_str(), name.c_str());
		_exitCode = exitCode;
	}
}

bool ShaderTool::printInfo() {
	for (const auto& block : _shaderStruct.uniformBlocks) {
		Log::debug("Found uniform block %s with %i members", block.name.c_str(), int(block.members.size()));
	}
	for (const auto& v : _shaderStruct.uniforms) {
		Log::debug("Found uniform of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.attributes) {
		Log::debug("Found attribute of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.varyings) {
		Log::debug("Found varying of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.outs) {
		Log::debug("Found out var of type %i with name %s", int(v.type), v.name.c_str());
	}

	const bool printIncludes = hasArg("--printincludes");
	if (printIncludes) {
		for (const core::String& i : _includes) {
			Log::info("%s%s", _shaderpath.c_str(), i.c_str());
		}
		return false;
	}
	return true;
}

std::pair<std::string, bool> ShaderTool::getSource(const core::String& file) const {
	const io::FilesystemPtr& fs = filesystem();

	const std::pair<std::string, bool>& retIncludes = util::handleIncludes(fs->load(file), _includeDirs);
	core::String src = retIncludes.first;
	int level = 0;
	bool success = retIncludes.second;
	while (core::string::contains(src, "#include")) {
		const std::pair<std::string, bool>& ret = util::handleIncludes(src, _includeDirs);
		src = ret.first;
		success &= ret.second;
		++level;
		if (level >= 10) {
			Log::warn("Abort shader include loop for %s", file.c_str());
			break;
		}
	}
	return std::make_pair(src, success);
}

core::AppState ShaderTool::onRunning() {
	const core::String& shaderfile         = getArgVal("--shader");
	const bool printIncludes              = hasArg("--printincludes");
	if (!printIncludes) {
		_glslangValidatorBin              = getArgVal("--glslang");
		_headerTemplateFile               = getArgVal("--headertemplate");
		_sourceTemplateFile               = getArgVal("--sourcetemplate");
		_uniformBufferTemplateFile        = getArgVal("--buffertemplate");
		_namespaceSrc                     = getArgVal("--namespace");
		_shaderDirectory                  = getArgVal("--shaderdir");
		_sourceDirectory                  = getArgVal("--sourcedir",
				_filesystem->basePath() + "src/modules/" + _namespaceSrc + "/");
		_postfix                          = getArgVal("--postfix", "");

		// handle include dirs
		_includeDirs.push_back(".");
		int index = 0;
		for (;;) {
			const core::String& dir = getArgVal("-I", "", &index);
			if (dir.empty()) {
				break;
			}
			_includeDirs.push_back(dir);
		}

		if (!core::string::endsWith(_shaderDirectory, "/")) {
			_shaderDirectory = _shaderDirectory + "/";
		}
		Log::debug("Using glslangvalidator binary: %s", _glslangValidatorBin.c_str());
		Log::debug("Using %s as output directory", _sourceDirectory.c_str());
		Log::debug("Using %s as namespace", _namespaceSrc.c_str());
		Log::debug("Using %s as shader directory", _shaderDirectory.c_str());
	}

	Log::debug("Preparing shader file %s", shaderfile.c_str());
	_shaderfile = std::string(core::string::extractFilename(shaderfile.c_str()));
	Log::debug("Preparing shader file %s", _shaderfile.c_str());
	const io::FilesystemPtr& fs = filesystem();
	_shaderpath = std::string(core::string::extractPath(shaderfile.c_str()));
	const bool changedDir = fs->pushDir(_shaderpath);

	video::Shader shader;

	_shaderStruct.filename = _shaderfile;
	_shaderStruct.name = _shaderfile;

	const core::String& writePath = fs->homePath();
	Log::debug("Writing shader file %s to %s", _shaderfile.c_str(), writePath.c_str());

	const core::String& templateShaderHeader = fs->load(_headerTemplateFile);
	const core::String& templateShaderSource = fs->load(_sourceTemplateFile);
	const core::String& templateUniformBuffer = fs->load(_uniformBufferTemplateFile);

	const core::String& computeFilename = _shaderfile + COMPUTE_POSTFIX;
	std::pair<std::string, bool> computeBuffer = getSource(computeFilename);
	if (!computeBuffer.first.empty()) {
		if (!computeBuffer.second) {
			Log::error("Failed to parse compute shader %s", _shaderfile.c_str());
			_exitCode = 1;
			return core::AppState::Cleanup;
		}
		const core::String& computeSrcSource = shader.getSource(video::ShaderType::Compute, computeBuffer.first, false, &_includes);
		if (!parse(computeSrcSource, false)) {
			Log::error("Failed to parse compute shader %s", _shaderfile.c_str());
			_exitCode = 1;
			return core::AppState::Cleanup;
		}

		if (!printInfo()) {
			return core::AppState::Cleanup;
		}

		if (!shadertool::generateSrc(templateShaderHeader, templateShaderSource, templateUniformBuffer, _shaderStruct,
				filesystem(), _namespaceSrc, _sourceDirectory, _shaderDirectory, _postfix,
				"", "", "", computeBuffer.first)) {
			Log::error("Failed to generate shader source for %s", _shaderfile.c_str());
			_exitCode = 1;
			return core::AppState::Cleanup;
		}

		const core::String& computeSource = shader.getSource(video::ShaderType::Compute, computeBuffer.first, true);
		const core::String& finalComputeFilename = _appname + "-" + computeFilename;
		fs->write(finalComputeFilename, computeSource);

		Log::debug("Validating shader file %s", _shaderfile.c_str());
		validate(finalComputeFilename);
		return core::AppState::Cleanup;
	}

	const core::String& fragmentFilename = _shaderfile + FRAGMENT_POSTFIX;
	const std::pair<std::string, bool>& fragmentBuffer = getSource(fragmentFilename);
	if (fragmentBuffer.first.empty() || !fragmentBuffer.second) {
		Log::error("Could not load %s", fragmentFilename.c_str());
		_exitCode = 127;
		return core::AppState::Cleanup;
	}

	const core::String& vertexFilename = _shaderfile + VERTEX_POSTFIX;
	const std::pair<std::string, bool>& vertexBuffer = getSource(vertexFilename);
	if (vertexBuffer.first.empty() || !vertexBuffer.second) {
		Log::error("Could not load %s", vertexFilename.c_str());
		_exitCode = 127;
		return core::AppState::Cleanup;
	}

	const core::String& geometryFilename = _shaderfile + GEOMETRY_POSTFIX;
	const std::pair<std::string, bool>& geometryBuffer = getSource(geometryFilename);

	const core::String& fragmentSrcSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer.first, false, &_includes);
	const core::String& vertexSrcSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer.first, false, &_includes);

	if (!parse(fragmentSrcSource, false)) {
		Log::error("Failed to parse fragment shader %s", _shaderfile.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}
	if (!parse(vertexSrcSource, true)) {
		Log::error("Failed to parse vertex shader %s", _shaderfile.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}
	if (!geometryBuffer.first.empty()) {
		if (!geometryBuffer.second) {
			Log::error("Failed to parse geometry shader %s", _shaderfile.c_str());
			_exitCode = 1;
			return core::AppState::Cleanup;
		}
		const core::String& geometrySrcSource = shader.getSource(video::ShaderType::Geometry, geometryBuffer.first, false, &_includes);
		if (!parse(geometrySrcSource, false)) {
			Log::error("Failed to parse geometry shader %s", _shaderfile.c_str());
			_exitCode = 1;
			return core::AppState::Cleanup;
		}
	}

	if (!printInfo()) {
		return core::AppState::Cleanup;
	}

	if (!shadertool::generateSrc(templateShaderHeader, templateShaderSource, templateUniformBuffer, _shaderStruct,
			filesystem(), _namespaceSrc, _sourceDirectory, _shaderDirectory, _postfix,
			vertexBuffer.first, geometryBuffer.first, fragmentBuffer.first, computeBuffer.first)) {
		Log::error("Failed to generate shader source for %s", _shaderfile.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}
	const core::String& fragmentSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer.first, true);
	const core::String& vertexSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer.first, true);
	const core::String& geometrySource = shader.getSource(video::ShaderType::Geometry, geometryBuffer.first, true);

	if (changedDir) {
		fs->popDir();
	}

	const core::String& finalFragmentFilename = _appname + "-" + fragmentFilename;
	const core::String& finalVertexFilename = _appname + "-" + vertexFilename;
	const core::String& finalGeometryFilename = _appname + "-" + geometryFilename;
	fs->write(finalFragmentFilename, fragmentSource);
	fs->write(finalVertexFilename, vertexSource);
	if (!geometrySource.empty()) {
		fs->write(finalGeometryFilename, geometrySource);
	}

	Log::debug("Validating shader file %s", _shaderfile.c_str());

	validate(finalFragmentFilename);
	validate(finalVertexFilename);
	if (!geometrySource.empty()) {
		validate(finalGeometryFilename);
	}

	return core::AppState::Cleanup;
}

CONSOLE_APP(ShaderTool)
