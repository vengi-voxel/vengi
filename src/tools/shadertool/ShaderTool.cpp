/**
 * @file
 */

#include "ShaderTool.h"
#include "io/Filesystem.h"
#include "core/Process.h"
#include "core/String.h"
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
}

bool ShaderTool::parse(const std::string& buffer, bool vertex) {
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

void ShaderTool::validate(const std::string& name) {
	const std::string& writePath = filesystem()->homePath();
	std::vector<std::string> args;
	args.push_back(writePath + name);
	char output[4096] = "";
	int exitCode = core::Process::exec(_glslangValidatorBin, args, nullptr, sizeof(output), output);
	if (exitCode != 0) {
		Log::error("Failed to validate shader '%s'", name.c_str());
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
		for (const std::string& i : _includes) {
			Log::info("%s%s", _shaderpath.c_str(), i.c_str());
		}
		return false;
	}
	return true;
}

std::string ShaderTool::getSource(const std::string& file) const {
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

core::AppState ShaderTool::onRunning() {
	const std::string& shaderfile         = getArgVal("--shader");
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
			const std::string& dir = getArgVal("-I", "", &index);
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

	const std::string& writePath = fs->homePath();
	Log::debug("Writing shader file %s to %s", _shaderfile.c_str(), writePath.c_str());

	const std::string& templateShaderHeader = fs->load(_headerTemplateFile);
	const std::string& templateShaderSource = fs->load(_sourceTemplateFile);
	const std::string& templateUniformBuffer = fs->load(_uniformBufferTemplateFile);

	const std::string& computeFilename = _shaderfile + COMPUTE_POSTFIX;
	const std::string& computeBuffer = getSource(computeFilename);
	if (!computeBuffer.empty()) {
		const std::string& computeSrcSource = shader.getSource(video::ShaderType::Compute, computeBuffer, false, &_includes);
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
				"", "", "", computeBuffer)) {
			Log::error("Failed to generate shader source for %s", _shaderfile.c_str());
			_exitCode = 1;
			return core::AppState::Cleanup;
		}

		const std::string& computeSource = shader.getSource(video::ShaderType::Compute, computeBuffer, true);
		const std::string& finalComputeFilename = _appname + "-" + computeFilename;
		fs->write(finalComputeFilename, computeSource);

		Log::debug("Validating shader file %s", _shaderfile.c_str());
		validate(finalComputeFilename);
		return core::AppState::Cleanup;
	}

	const std::string& fragmentFilename = _shaderfile + FRAGMENT_POSTFIX;
	const std::string& fragmentBuffer = getSource(fragmentFilename);
	if (fragmentBuffer.empty()) {
		Log::error("Could not load %s", fragmentFilename.c_str());
		_exitCode = 127;
		return core::AppState::Cleanup;
	}

	const std::string& vertexFilename = _shaderfile + VERTEX_POSTFIX;
	const std::string& vertexBuffer = getSource(vertexFilename);
	if (vertexBuffer.empty()) {
		Log::error("Could not load %s", vertexFilename.c_str());
		_exitCode = 127;
		return core::AppState::Cleanup;
	}

	const std::string& geometryFilename = _shaderfile + GEOMETRY_POSTFIX;
	const std::string& geometryBuffer = getSource(geometryFilename);

	const std::string& fragmentSrcSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer, false, &_includes);
	const std::string& vertexSrcSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer, false, &_includes);

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
	if (!geometryBuffer.empty()) {
		const std::string& geometrySrcSource = shader.getSource(video::ShaderType::Geometry, geometryBuffer, false, &_includes);
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
			vertexBuffer, geometryBuffer, fragmentBuffer, computeBuffer)) {
		Log::error("Failed to generate shader source for %s", _shaderfile.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}
	const std::string& fragmentSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer, true);
	const std::string& vertexSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer, true);
	const std::string& geometrySource = shader.getSource(video::ShaderType::Geometry, geometryBuffer, true);

	if (changedDir) {
		fs->popDir();
	}

	const std::string& finalFragmentFilename = _appname + "-" + fragmentFilename;
	const std::string& finalVertexFilename = _appname + "-" + vertexFilename;
	const std::string& finalGeometryFilename = _appname + "-" + geometryFilename;
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
